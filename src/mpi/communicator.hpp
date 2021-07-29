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
class communicator::impl : public communicator_base<communicator::impl>
{
  public:
    context_impl*                 m_context;
    callback_queue<request::impl> m_callbacks;

    impl(context_impl* ctxt)
    : communicator_base(ctxt)
    , m_context(ctxt)
    {
    }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    request::impl send(
        context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst, tag_type tag)
    {
        MPI_Request r;
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
        return {r};
    }

    void send(detail::message_buffer&& msg, std::size_t size, rank_type dst, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    void recv(detail::message_buffer&& msg, std::size_t size, rank_type src, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    void progress() { m_callbacks.progress(); }
};

} // namespace oomph
