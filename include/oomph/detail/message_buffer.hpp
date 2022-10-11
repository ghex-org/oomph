/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <utility>

#include <oomph/config.hpp>
#include <oomph/util/pimpl.hpp>
#include <oomph/util/heap_pimpl.hpp>

namespace oomph
{
namespace detail
{
class message_buffer
{
  public:
    class heap_ptr_impl;
    //using pimpl = util::pimpl<heap_ptr_impl, 64, 8>;
    using pimpl = util::heap_pimpl<heap_ptr_impl>;

  public:
    void* m_ptr = nullptr;
    pimpl m_heap_ptr;

  public:
    ~message_buffer();

    message_buffer() noexcept = default;

    template<typename VoidPtr>
    message_buffer(VoidPtr ptr)
    : m_ptr{ptr.get()}
    , m_heap_ptr(ptr)
    {
    }

    message_buffer(message_buffer&& other) noexcept
    : m_ptr{std::exchange(other.m_ptr, nullptr)}
    , m_heap_ptr{std::move(other.m_heap_ptr)}
    {
    }

    message_buffer& operator=(message_buffer&&);

    operator bool() const noexcept { return m_ptr; }

    bool on_device() const noexcept;

#if OOMPH_ENABLE_DEVICE
    void*       device_data() noexcept;
    void const* device_data() const noexcept;

    int device_id() const noexcept;

    void clone_to_device(std::size_t count);
    void clone_to_host(std::size_t count);
#endif

    void clear();
};

} // namespace detail

} // namespace oomph
