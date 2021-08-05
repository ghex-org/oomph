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
#include "./request.hpp"
#include "./context.hpp"
#include "../communicator_base.hpp"
#include "../callback_utils.hpp"
#include "../device_guard.hpp"

namespace oomph
{
class communicator_impl : public communicator_base<communicator_impl>
{
    using rank_type = communicator::rank_type;
    using tag_type = communicator::tag_type;

  public:
    context_impl* m_context;
    //callback_queue<request::impl> m_send_callbacks;
    //callback_queue<request::impl> m_recv_callbacks;
    callback_queue2<request::impl, send_request::data_type> m_send_callbacks2;
    callback_queue2<request::impl, recv_request::data_type> m_recv_callbacks2;

    communicator_impl(context_impl* ctxt)
    : communicator_base(ctxt)
    , m_context(ctxt)
    {
    }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    request::impl send(
        context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst, tag_type tag)
    {
        MPI_Request        r;
        const_device_guard dg(ptr);
        OOMPH_CHECK_MPI_RESULT(MPI_Isend(dg.data(), size, MPI_BYTE, dst, tag, mpi_comm(), &r));
        return {r};
    }

    request::impl recv(
        context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src, tag_type tag)
    {
        MPI_Request  r;
        device_guard dg(ptr);
        OOMPH_CHECK_MPI_RESULT(MPI_Irecv(dg.data(), size, MPI_BYTE, src, tag, mpi_comm(), &r));
        return {r, false};
    }

    //void send(detail::message_buffer&& msg, std::size_t size, rank_type dst, tag_type tag,
    //    std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    //void recv(detail::message_buffer&& msg, std::size_t size, rank_type src, tag_type tag,
    //    std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    void send2(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        tag_type tag, util::unique_function<void()>&& cb,
        std::shared_ptr<send_request::data_type>&& h)
    {
        auto req = send(ptr, size, dst, tag);
        if (req.is_ready()) cb();
        else
            m_send_callbacks2.enqueue(std::move(req), std::move(cb), std::move(h));
    }

    void recv2(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src, tag_type tag,
        util::unique_function<void()>&& cb, std::shared_ptr<recv_request::data_type>&& h)
    {
        auto req = recv(ptr, size, src, tag);
        if (req.is_ready()) cb();
        else
            m_recv_callbacks2.enqueue(std::move(req), std::move(cb), std::move(h));
    }

    //void recv(detail::message_buffer& msg, std::size_t size, rank_type src, tag_type tag,
    //    std::function<void(rank_type, tag_type)>&& cb);

    //bool cancel_recv_cb(rank_type src, tag_type tag,
    //    std::function<void(detail::message_buffer, std::size_t size, rank_type, tag_type)>&& cb)
    //{
    //    bool done = false;
    //    m_recv_callbacks.dequeue(src, tag,
    //        [&done, cb = std::move(cb), src, tag](
    //            request::impl&& req, detail::message_buffer&& msg, std::size_t size) {
    //            if (req.cancel())
    //            {
    //                cb(std::move(msg), size, src, tag);
    //                done = true;
    //                return true;
    //            }
    //            return false;
    //        });
    //    return done;
    //}

    void progress()
    {
        m_send_callbacks2.progress();
        m_recv_callbacks2.progress();
    }

    bool cancel_recv_cb(std::size_t index) { return m_recv_callbacks2.cancel(index); }
};

} // namespace oomph
