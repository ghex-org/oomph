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

#include <chrono>
#include <thread>

#include <nccl.h>

#include <oomph/context.hpp>

// paths relative to backend
#include "../communicator_base.hpp"
#include "../device_guard.hpp"
#include "./context.hpp"
#include "./request.hpp"
#include "./request_state.hpp"
#include "./request_queue.hpp"

namespace oomph
{
class communicator_impl : public communicator_base<communicator_impl>
{
  public:
    context_impl* m_context;
    request_queue m_send_reqs;
    request_queue m_recv_reqs;
    bool m_in_group = false;
    std::optional<cudaEvent_t> m_group_event;
    cudaStream_t m_last_stream;

    communicator_impl(context_impl* ctxt)
    : communicator_base(ctxt)
    , m_context(ctxt)
    {
    }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    bool is_stream_aware() const noexcept { return true; }

    void start_group() {
      OOMPH_CHECK_NCCL_RESULT(ncclGroupStart());
      m_in_group = true;

      // TODO: Correct flags etc.
      cudaEvent_t event;
      cudaEventCreate(&event);
      std::cerr << "created group event " << event << "\n";
      m_group_event = event;
    }

    void end_group() {
      m_in_group = false;
      OOMPH_CHECK_NCCL_RESULT(ncclGroupEnd());

      // All streams used in a NCCL group synchronize with the end of the group.
      // We arbitrarily pick the last stream to synchronize against.
      OOMPH_CHECK_CUDA_RESULT(cudaEventRecord(m_group_event.value(), m_last_stream));
      // TODO: Release event.
    }

    nccl_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        [[maybe_unused]] tag_type tag, void* stream)
    {
        std::cerr << "nccl::send\n";

        const_device_guard dg(ptr);
        OOMPH_CHECK_NCCL_RESULT(
            ncclSend(dg.data(), size, ncclChar, dst, m_context->get_comm(), static_cast<cudaStream_t>(stream)));

        if (m_in_group) {
            m_last_stream = static_cast<cudaStream_t>(stream);
            // Store event now, but record it when group ends
            // TODO: Have to make sure it's safe to query event early.
            std::cerr << "using group event " << m_group_event.value() << "\n";
            return {m_group_event.value()};
        } else {
            // TODO: Correct flags etc.
            // TODO: Free event.
            cudaEvent_t event;
            cudaEventCreate(&event);
            OOMPH_CHECK_CUDA_RESULT(cudaEventRecord(event, static_cast<cudaStream_t>(stream)));
            return {event};
        }
    }

    nccl_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        [[maybe_unused]] tag_type tag, void* stream)
    {
        std::cerr << "nccl::recv\n";

        device_guard dg(ptr);
        OOMPH_CHECK_NCCL_RESULT(
            ncclRecv(dg.data(), size, ncclChar, src, m_context->get_comm(), static_cast<cudaStream_t>(stream)));

        if (m_in_group) {
            m_last_stream = static_cast<cudaStream_t>(stream);
            // Store event now, but record it when group ends
            std::cerr << "using group event " << m_group_event.value() << "\n";
            return {m_group_event.value()};
        } else {
            // TODO: Correct flags etc.
            // TODO: Free event.
            cudaEvent_t event;
            cudaEventCreate(&event);
            OOMPH_CHECK_CUDA_RESULT(cudaEventRecord(event, static_cast<cudaStream_t>(stream)));
            return {event};
        }
    }

    send_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb, std::size_t* scheduled, void* stream)
    {
        auto req = send(ptr, size, dst, tag, stream);
        // TODO: Do early checking?
        auto s = m_req_state_factory.make(m_context, this, scheduled, dst, tag, std::move(cb), req);
        s->create_self_ref();
        m_send_reqs.enqueue(s.get());
        return {std::move(s)};
    }

    recv_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb, std::size_t* scheduled, void* stream)
    {
        auto req = recv(ptr, size, src, tag, stream);
        // TODO: Do early checking?
        auto s = m_req_state_factory.make(m_context, this, scheduled, src, tag, std::move(cb), req);
        s->create_self_ref();
        m_recv_reqs.enqueue(s.get());
        return {std::move(s)};
    }

    shared_recv_request shared_recv(context_impl::heap_type::pointer& ptr, std::size_t size,
        rank_type src, tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb,
        std::atomic<std::size_t>* scheduled, void* stream)
    {
        auto req = recv(ptr, size, src, tag, stream);
        // TODO: Do early checking?
        auto s = std::make_shared<detail::shared_request_state>(m_context, this, scheduled, src,
            tag, std::move(cb), req);
        s->create_self_ref();
        m_context->m_req_queue.enqueue(s.get());
        return {std::move(s)};
    }

    void progress()
    {
        std::cerr << "nccl communicator::progress\n";
	// Communication progresses independently, but requests must be marked
	// ready and callbacks must be invoked.
        m_send_reqs.progress();
        m_recv_reqs.progress();
        m_context->progress();
        // std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    bool cancel_recv(detail::request_state*)
    {
        // TODO: NCCL does not allow cancellation?
        return false;
    }
};

} // namespace oomph
