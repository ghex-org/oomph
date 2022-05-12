/*
 * ghex-org
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
#include "./callback_queue.hpp"
#include "./context.hpp"
#include "../communicator_base.hpp"
#include "../device_guard.hpp"

namespace oomph
{
class communicator_impl : public communicator_base<communicator_impl>
{
    using rank_type = communicator::rank_type;
    using tag_type = communicator::tag_type;

  public:
    context_impl*  m_context;
    callback_queue m_send_callbacks;
    callback_queue m_recv_callbacks;

    communicator_impl(context_impl* ctxt)
    : communicator_base(ctxt)
    , m_context(ctxt)
    {
    }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    mpi_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        tag_type tag)
    {
        MPI_Request        r;
        const_device_guard dg(ptr);
        OOMPH_CHECK_MPI_RESULT(MPI_Isend(dg.data(), size, MPI_BYTE, dst, tag, mpi_comm(), &r));
        return {r};
    }

    mpi_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        tag_type tag)
    {
        MPI_Request  r;
        device_guard dg(ptr);
        OOMPH_CHECK_MPI_RESULT(MPI_Irecv(dg.data(), size, MPI_BYTE, src, tag, mpi_comm(), &r));
        return {r};
    }

    void send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        tag_type tag, util::unique_function<void()>&& cb, communicator::shared_request_ptr&& h)
    {
        auto req = send(ptr, size, dst, tag);
        if (req.is_ready()) cb();
        else
            m_send_callbacks.enqueue(req, std::move(cb), std::move(h));
    }

    void recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src, tag_type tag,
        util::unique_function<void()>&& cb, communicator::shared_request_ptr&& h)
    {
        auto req = recv(ptr, size, src, tag);
        if (req.is_ready()) cb();
        else
            m_recv_callbacks.enqueue(req, std::move(cb), std::move(h));
    }

    void shared_recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src, tag_type tag,
        util::unique_function<void()>&& cb/*, communicator::shared_request_ptr&& h*/)
    {
        auto req = recv(ptr, size, src, tag);
        if (req.is_ready()) cb();
        else
            //m_recv_callbacks.enqueue(req, std::move(cb), std::move(h));
            m_context->m_cb_queue.enqueue(req, std::move(cb));
    }

    std::size_t scheduled_shared_recvs() const noexcept { return m_context->m_cb_queue.size(); }

    void progress()
    {
        m_send_callbacks.progress();
        m_recv_callbacks.progress();
        m_context->m_cb_queue.progress();
    }

    bool cancel_recv_cb(recv_request const& req)
    {
        return m_recv_callbacks.cancel(req.m_data->reserved()->m_index);
    }
};

} // namespace oomph
