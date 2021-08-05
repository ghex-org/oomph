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
    return c.make_region(ptr, size, true);
}
#endif

//template<>
//request::request<>()
//: m_impl()
//{
//}

request::impl::~impl()
{
    if (m_req) destroy();
}

communicator_impl*
context_impl::get_communicator()
{
    auto comm = new communicator_impl{this, m_worker.get(),
        std::make_unique<worker_type>(
            get(), m_db /*, m_mutex*/, UCS_THREAD_MODE_SERIALIZED /*, m_rank_topology*/),
        m_mutex};
    m_comms_set.insert(comm);
    return comm;
}

} // namespace oomph

#include "../src.cpp"

namespace oomph
{
//void
//communicator_impl::send(detail::message_buffer&& msg, std::size_t size, rank_type dst, tag_type tag,
//    std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb)
//{
//    auto req = send(msg.m_heap_ptr->m, size, dst, tag);
//    if (req.is_ready()) cb(std::move(msg), dst, tag);
//    else
//        m_send_callbacks.enqueue(std::move(msg), size, dst, tag, std::move(req), std::move(cb));
//}
//void
//communicator_impl::recv(detail::message_buffer&& msg, std::size_t size, rank_type src, tag_type tag,
//    std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb)
//{
//    auto req = recv(msg.m_heap_ptr->m, size, src, tag);
//    if (req.is_ready()) cb(std::move(msg), src, tag);
//    else
//        m_recv_callbacks.enqueue(std::move(msg), size, src, tag, std::move(req), std::move(cb));
//}

void
communicator_impl::send2(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
    tag_type tag, util::unique_function<void()>&& cb, std::shared_ptr<send_cb_handle::data_type>&& h)
{
    auto req = send(ptr, size, dst, tag);
    if (req.is_ready()) cb();
    else
        m_send_callbacks2.enqueue(std::move(req), std::move(cb), std::move(h));
}

void
communicator_impl::recv2(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
    tag_type tag, util::unique_function<void()>&& cb, std::shared_ptr<recv_cb_handle::data_type>&& h)
{
    auto req = recv(ptr, size, src, tag);
    if (req.is_ready()) cb();
    else
        m_recv_callbacks2.enqueue(std::move(req), std::move(cb), std::move(h));
}
} // namespace oomph
