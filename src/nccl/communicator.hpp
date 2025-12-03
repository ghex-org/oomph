/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <nccl.h>

#include <oomph/context.hpp>

// paths relative to backend
#include <../communicator_base.hpp>
#include <../device_guard.hpp>
#include <context.hpp>
// #include <request_queue.hpp>
#include <request.hpp>
#include <request_state.hpp>

namespace oomph
{
class communicator_impl : public communicator_base<communicator_impl>
{
  public:
    context_impl* m_context;
    // request_queue m_send_reqs;
    // request_queue m_recv_reqs;

    communicator_impl(context_impl* ctxt)
    : communicator_base(ctxt)
    , m_context(ctxt)
    {
    }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    bool is_stream_aware() const noexcept { return true; }

    void start_group() { OOMPH_CHECK_NCCL_RESULT(ncclGroupStart()); }

    void end_group() { OOMPH_CHECK_NCCL_RESULT(ncclGroupEnd()); }

    nccl_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        [[maybe_unused]] tag_type tag, void* stream)
    {
        const_device_guard dg(ptr);
        OOMPH_CHECK_NCCL_RESULT(
            ncclSend(dg.data(), size, ncclChar, dst, m_context->get_comm(), static_cast<cudaStream_t>(stream)));
        // TODO: Return event to stream? Return void?
        return {};
    }

    nccl_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        [[maybe_unused]] tag_type tag, void* stream)
    {
        device_guard dg(ptr);
        OOMPH_CHECK_NCCL_RESULT(
            ncclRecv(dg.data(), size, ncclChar, src, m_context->get_comm(), static_cast<cudaStream_t>(stream)));
        // TODO: Return event to stream? Return void?
        return {};
    }

    send_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb, std::size_t* scheduled, void* stream)
    {
        auto req = send(ptr, size, dst, tag, stream);
        // if (!has_reached_recursion_depth() && req.is_ready())
        // {
        //     auto inc = recursion();
        //     cb(dst, tag);
        //     return {};
        // }
        // else
        // {
        // TODO: Do we want to support callbacks for NCCL communication? How should this be structured?
        auto s = m_req_state_factory.make(m_context, this, scheduled, dst, tag, std::move(cb), req);
        // s->create_self_ref();
        // TODO: Callback ignored.
        // TODO: Have to respect `scheduled`. Needs to be incremented before
        // send and decremeted when done.
        // m_send_reqs.enqueue(s.get());
        return {std::move(s)};
        // }
    }

    recv_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb, std::size_t* scheduled, void* stream)
    {
        auto req = recv(ptr, size, src, tag, stream);
        // if (!has_reached_recursion_depth() && req.is_ready())
        // {
        //     auto inc = recursion();
        //     cb(src, tag);
        //     return {};
        // }
        // else
        // {
        auto s = m_req_state_factory.make(m_context, this, scheduled, src, tag, std::move(cb), req);
        // s->create_self_ref();
        // m_recv_reqs.enqueue(s.get());
        return {std::move(s)};
        // }
    }

    shared_recv_request shared_recv(context_impl::heap_type::pointer& ptr, std::size_t size,
        rank_type src, tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb,
        std::atomic<std::size_t>* scheduled, void* stream)
    {
        auto req = recv(ptr, size, src, tag, stream);
        // if (!m_context->has_reached_recursion_depth() && req.is_ready())
        // {
        //     auto inc = m_context->recursion();
        //     cb(src, tag);
        //     return {};
        // }
        // else
        // {
        auto s = std::make_shared<detail::shared_request_state>(m_context, this, scheduled, src,
            tag, std::move(cb), req);
        // s->create_self_ref();
        // m_context->m_req_queue.enqueue(s.get());
        return {std::move(s)};
        // }
    }

    void progress()
    {
        // Nothing to do to progress NCCL. Just wait for GPU to finish.
    }

    bool cancel_recv(detail::request_state*)
    {
        // TODO: NCCL does not allow cancellation?
        return false;
    }
};

} // namespace oomph
