/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/util/pimpl.hpp>

namespace oomph
{
namespace detail
{
class message_buffer
{
  public:
    class heap_ptr_impl;
    using pimpl = util::pimpl<heap_ptr_impl, 64, 8>;

  public:
    void* m_ptr = nullptr;
    pimpl m_heap_ptr;

  public:
    message_buffer();
    template<typename VoidPtr>
    message_buffer(VoidPtr ptr);
    message_buffer(message_buffer&&);
    ~message_buffer();
    message_buffer& operator=(message_buffer&&);

    operator bool() const noexcept { return m_ptr; }

    bool on_device() const noexcept;

#if HWMALLOC_ENABLE_DEVICE
    void*       device_data() noexcept;
    void const* device_data() const noexcept;

    int device_id() const noexcept;
#endif

    void clear();
};

} // namespace detail

} // namespace oomph
