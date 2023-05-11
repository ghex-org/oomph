/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <hwmalloc/numa.hpp>
#include <oomph/config.hpp>

// paths relative to backend
#include <context.hpp>
#include <communicator.hpp>
#include <../message_buffer.hpp>
#include <../util/heap_pimpl_src.hpp>

OOMPH_INSTANTIATE_HEAP_PIMPL(oomph::detail::message_buffer::heap_ptr_impl)

namespace oomph
{

rank_type
communicator::rank() const noexcept
{
    return m_state->m_impl->rank();
}

rank_type
communicator::size() const noexcept
{
    return m_state->m_impl->size();
}

bool
communicator::is_local(rank_type rank) const noexcept
{
    return m_state->m_impl->is_local(rank);
}

MPI_Comm
communicator::mpi_comm() const noexcept
{
    return m_state->m_impl->mpi_comm();
}

void
communicator::progress()
{
    m_state->m_impl->progress();
}

send_request
communicator::send(detail::message_buffer::heap_ptr_impl const* m_ptr, std::size_t size,
    rank_type dst, tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb)
{
    return m_state->m_impl->send(m_ptr->m, size, dst, tag, std::move(cb),
        &(m_state->scheduled_sends));
}

recv_request
communicator::recv(detail::message_buffer::heap_ptr_impl* m_ptr, std::size_t size, rank_type src,
    tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb)
{
    return m_state->m_impl->recv(m_ptr->m, size, src, tag, std::move(cb),
        &(m_state->scheduled_recvs));
}

shared_recv_request
communicator::shared_recv(detail::message_buffer::heap_ptr_impl* m_ptr, std::size_t size,
    rank_type src, tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb)
{
    return m_state->m_impl->shared_recv(m_ptr->m, size, src, tag, std::move(cb),
        m_state->m_shared_scheduled_recvs);
}

detail::message_buffer
communicator::make_buffer_core(std::size_t size)
{
    return m_state->m_impl->get_heap().allocate(size, hwmalloc::numa().local_node());
}

detail::message_buffer
communicator::make_buffer_core(void* ptr, std::size_t size)
{
    return m_state->m_impl->get_heap().register_user_allocation(ptr, size);
}

#if OOMPH_ENABLE_DEVICE
detail::message_buffer
communicator::make_buffer_core(std::size_t size, int id)
{
    return m_state->m_impl->get_heap().allocate(size, hwmalloc::numa().local_node(), id);
}

detail::message_buffer
communicator::make_buffer_core(void* device_ptr, std::size_t size, int device_id)
{
    return m_state->m_impl->get_heap().register_user_allocation(device_ptr, device_id, size);
}

detail::message_buffer
communicator::make_buffer_core(void* ptr, void* device_ptr, std::size_t size, int device_id)
{
    return m_state->m_impl->get_heap().register_user_allocation(ptr, device_ptr, device_id, size);
}
#endif
} // namespace oomph
