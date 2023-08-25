/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <oomph/config.hpp>

// paths relative to backend
#include <../message_buffer.hpp>
#include <../util/heap_pimpl_src.hpp>

OOMPH_INSTANTIATE_HEAP_PIMPL(oomph::detail::message_buffer::heap_ptr_impl)

namespace oomph
{
namespace detail
{

message_buffer::~message_buffer()
{
    if (m_ptr) m_heap_ptr->release();
}

message_buffer&
message_buffer::operator=(message_buffer&& other)
{
    if (m_ptr) m_heap_ptr->release();
    m_ptr = std::exchange(other.m_ptr, nullptr);
    m_heap_ptr = std::move(other.m_heap_ptr);
    return *this;
}

bool
message_buffer::on_device() const noexcept
{
    return m_heap_ptr->m.on_device();
}

#if OOMPH_ENABLE_DEVICE
void*
message_buffer::device_data() noexcept
{
    return m_heap_ptr->m.device_ptr();
}

void const*
message_buffer::device_data() const noexcept
{
    return m_heap_ptr->m.device_ptr();
}

int
message_buffer::device_id() const noexcept
{
    return m_heap_ptr->m.device_id();
}

void
message_buffer::clone_to_device(std::size_t count)
{
    hwmalloc::memcpy_to_device(m_heap_ptr->m.device_ptr(), m_ptr, count);
}

void
message_buffer::clone_to_host(std::size_t count)
{
    hwmalloc::memcpy_to_host(m_ptr, m_heap_ptr->m.device_ptr(), count);
}
#endif

void
message_buffer::clear()
{
    m_ptr = nullptr;
    m_heap_ptr = context_impl::heap_type::pointer{nullptr};
}

} // namespace detail
} // namespace oomph
