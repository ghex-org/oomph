/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "./context.hpp"
#include "./communicator.hpp"
#include "./message_buffer.hpp"
#include "./request.hpp"
//#include "./send_channel.hpp"
//#include "./recv_channel.hpp"

namespace oomph
{
template<>
region
register_memory<context_impl>(context_impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size);
}

#if HWMALLOC_ENABLE_DEVICE
template<>
region
register_device_memory<context_impl>(context_impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size);
}
#endif

//template<>
//request::request<MPI_Request>(MPI_Request r)
//: m_impl(r)
//{
//}

request::impl::~impl() = default;

communicator::impl*
context_impl::get_communicator()
{
    auto comm = new communicator::impl{this};
    m_comms_set.insert(comm);
    return comm;
}

//send_channel_base::send_channel_base(communicator& comm, std::size_t size, std::size_t T_size,
//    communicator::rank_type dst, communicator::tag_type tag, std::size_t levels)
//: m_impl(comm.m_impl, size, T_size, dst, tag, levels)
//{
//}
//
//recv_channel_base::recv_channel_base(communicator& comm, std::size_t size, std::size_t T_size,
//    communicator::rank_type src, communicator::tag_type tag, std::size_t levels)
//: m_impl(comm.m_impl, size, T_size, src, tag, levels)
//{
//}

} // namespace oomph

#include "../src.cpp"

namespace oomph
{
void
communicator::impl::send(detail::message_buffer&& msg, std::size_t size, rank_type dst,
    tag_type tag, std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb)
{
    auto req = send(msg.m_heap_ptr->m, size, dst, tag);
    m_callbacks.enqueue(std::move(msg), size, dst, tag, std::move(req), std::move(cb));
}
void
communicator::impl::recv(detail::message_buffer&& msg, std::size_t size, rank_type src,
    tag_type tag, std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb)
{
    auto req = recv(msg.m_heap_ptr->m, size, src, tag);
    m_callbacks.enqueue(std::move(msg), size, src, tag, std::move(req), std::move(cb));
}
} // namespace oomph
