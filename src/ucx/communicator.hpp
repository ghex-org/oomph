/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma once

#include <oomph/context.hpp>
#include <oomph/communicator.hpp>
#include "./request_data.hpp"
#include "./context.hpp"
#include "../communicator_base.hpp"
#include "../device_guard.hpp"
#include <boost/lockfree/queue.hpp>

namespace oomph
{
#define OOMPH_UCX_TAG_BITS             32
#define OOMPH_UCX_RANK_BITS            32
#define OOMPH_UCX_ANY_SOURCE_MASK      0x0000000000000000ul
#define OOMPH_UCX_SPECIFIC_SOURCE_MASK 0x00000000fffffffful
#define OOMPH_UCX_TAG_MASK             0xffffffff00000000ul

class communicator_impl : public communicator_base<communicator_impl>
{
  public:
    using worker_type = worker_t;
    using rank_type = communicator::rank_type;
    using tag_type = communicator::tag_type;

  public:
    context_impl*                                  m_context;
    worker_type*                                   m_recv_worker;
    std::unique_ptr<worker_type>                   m_send_worker;
    context_impl::mutex_t&                         m_mutex;
    boost::lockfree::queue<request_data::cb_ptr_t> m_recv_cb_queue;

  public:
    communicator_impl(context_impl* ctxt, worker_type* recv_worker,
        std::unique_ptr<worker_type>&& send_worker, context_impl::mutex_t& mtx)
    : communicator_base(ctxt)
    , m_context(ctxt)
    , m_recv_worker{recv_worker}
    , m_send_worker{std::move(send_worker)}
    , m_mutex{mtx}
    , m_recv_cb_queue(128)
    {
    }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    void progress()
    {
        while (ucp_worker_progress(m_send_worker->get())) {}
        // this is really important for large-scale multithreading: check if still is
        sched_yield();
        {
            // progress recv worker in locked region
            std::lock_guard<context_impl::mutex_t> lock(m_mutex);
            while (ucp_worker_progress(m_recv_worker->get())) {}
        }
        // work through ready recv callbacks, which were pushed to the queue by other threads
        // (including this thread)
        m_recv_cb_queue.consume_all([](request_data::cb_ptr_t cb) {
            cb->invoke();
            delete cb;
        });
    }

    void send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        tag_type tag, util::unique_function<void()>&& cb,
        std::shared_ptr<send_request::data_type>&& h)
    {
        const auto& ep = m_send_worker->connect(dst);
        const auto  stag =
            ((std::uint_fast64_t)tag << OOMPH_UCX_TAG_BITS) | (std::uint_fast64_t)(rank());

        ucs_status_ptr_t ret;
        {
            // device is set according to message memory: needed?
            const_device_guard dg(ptr);

            ret = ucp_tag_send_nb(ep.get(),         // destination
                dg.data(),                          // buffer
                size,                               // buffer size
                ucp_dt_make_contig(1),              // data type
                stag,                               // tag
                &communicator_impl::send_callback); // callback function pointer
        }

        if (reinterpret_cast<std::uintptr_t>(ret) == UCS_OK)
        {
            // send operation is completed immediately
            // call the callback
            cb();
            // request is freed by ucx internally
        }
        else if (!UCS_PTR_IS_ERR(ret))
        {
            // send operation was scheduled
            // attach necessary data to the request
            auto& req_data = request_data::get(ret);
            //req_data.m_ucx_ptr = ret; // probably not needed since set with request_init
            req_data.m_comm = this;
            req_data.m_cb = cb.release();
            h->m_data = &req_data;
        }
        else
        {
            // an error occurred
            throw std::runtime_error("oomph: ucx error - send operation failed");
        }
    }

    void recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src, tag_type tag,
        util::unique_function<void()>&& cb, std::shared_ptr<recv_request::data_type>&& h)
    {
        const auto rtag =
            (communicator::any_source == src)
                ? ((std::uint_fast64_t)tag << OOMPH_UCX_TAG_BITS)
                : ((std::uint_fast64_t)tag << OOMPH_UCX_TAG_BITS) | (std::uint_fast64_t)(src);

        const auto rtag_mask = (communicator::any_source == src)
                                   ? (OOMPH_UCX_TAG_MASK | OOMPH_UCX_ANY_SOURCE_MASK)
                                   : (OOMPH_UCX_TAG_MASK | OOMPH_UCX_SPECIFIC_SOURCE_MASK);

        // pointer to store callback in case of early completion
        request_data::cb_ptr_t cb_ptr = nullptr;
        {
            // locked region
            std::lock_guard<context_impl::mutex_t> lock(m_mutex);

            ucs_status_ptr_t ret;
            {
                // device is set according to message memory: needed?
                device_guard dg(ptr);

                ret = ucp_tag_recv_nb(m_recv_worker->get(), // worker
                    dg.data(),                              // buffer
                    size,                                   // buffer size
                    ucp_dt_make_contig(1),                  // data type
                    rtag,                                   // tag
                    rtag_mask,                              // tag mask
                    &communicator_impl::recv_callback);     // callback function pointer
            }

            if (!UCS_PTR_IS_ERR(ret))
            {
                if (UCS_INPROGRESS != ucp_request_check_status(ret))
                {
                    // early completed
                    // store callback for later: will be called outside locked region
                    cb_ptr = cb.release();
                    // destroy request
                    request_data::get(ret).clear();
                    ucp_request_free(ret);
                }
                else
                {
                    // recv operation was scheduled
                    // attach necessary data to the request
                    auto& req_data = request_data::get(ret);
                    //req_data.m_ucx_ptr = ret; // probably not needed since set with request_init
                    req_data.m_comm = this;
                    req_data.m_cb = cb.release();
                    h->m_data = &req_data;
                }
            }
            else
            {
                // an error occurred
                throw std::runtime_error("oomph: ucx error - recv operation failed");
            }
        }
        // check for early completion
        if (cb_ptr)
        {
            cb_ptr->invoke();
            delete cb_ptr;
        }
    }

    inline static void send_callback(void* ucx_req, ucs_status_t status)
    {
        auto& req_data = request_data::get(ucx_req);
        if (status == UCS_OK)
        {
            // invoke callback
            req_data.m_cb->invoke();
            delete req_data.m_cb;
        }
        // else: cancelled - do nothing - cancel for sends does not exist

        // destroy request
        req_data.clear();
        ucp_request_free(ucx_req);
    }

    void enqueue_recv(request_data::cb_ptr_t cb)
    {
        while (!m_recv_cb_queue.push(cb)) {}
    }

    inline static void recv_callback(
        void* ucx_req, ucs_status_t status, ucp_tag_recv_info_t* /*info*/)
    {
        auto& req_data = request_data::get(ucx_req);
        if (status == UCS_OK)
        {
            // return if early completion
            // early completion is indicated by missing request data (null pointer)
            if (!req_data.m_comm) return;

            // enqueue callback on the issuing communicator
            // this guarantees that only the communicator on which the receive was executed will
            // invoke the callback
            req_data.m_comm->enqueue_recv(req_data.m_cb);

            // destroy request
            req_data.clear();
            ucp_request_free(ucx_req);
        }
        else if (status == UCS_ERR_CANCELED)
        {
            // receive was cancelled: nothing to do
            // destroy request
            req_data.clear();
            ucp_request_free(ucx_req);
        }
        else
        {
            // an error occurred
            throw std::runtime_error("oomph: ucx error - recv message truncated");
        }
    }

    // Note: at this time, send requests cannot be canceled in UCX (1.7.0rc1)
    // https://github.com/openucx/ucx/issues/1162
    bool cancel_recv_cb(recv_request const& req)
    {
        auto& req_data = request_data::get(req.m_data->m_data);
        {
            // locked region
            std::lock_guard<context_impl::mutex_t> lock(m_mutex);
            ucp_request_cancel(m_recv_worker->get(), req_data.m_ucx_ptr);
        }
        delete req_data.m_cb;
        // The ucx callback will still be executed after the cancel. However, the status argument
        // will indicate whether the cancel was successful. Destruction/freeing of request is
        // handled in this callback.
        // Here return true regardless of whether the request was cancelled or not, since we do not
        // know at this point.
        return true;
    }
};

} // namespace oomph
