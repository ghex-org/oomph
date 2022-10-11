/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma once

#include <hwmalloc/device.hpp>
#include <oomph/config.hpp>

namespace oomph
{
struct device_guard_base
{
    bool m_on_device;
    int  m_new_device_id;
    int  m_current_device_id;

    device_guard_base(bool on_device = false, int new_id = 0)
    : m_on_device{on_device}
    , m_new_device_id{new_id}
    {
#if OOMPH_ENABLE_DEVICE
        m_current_device_id = hwmalloc::get_device_id();
        if (m_on_device && (m_current_device_id != m_new_device_id))
            hwmalloc::set_device_id(m_new_device_id);
#endif
    }

    device_guard_base(device_guard_base const&) = delete;

    ~device_guard_base()
    {
#if OOMPH_ENABLE_DEVICE
        if (m_on_device && (m_current_device_id != m_new_device_id))
            hwmalloc::set_device_id(m_current_device_id);
#endif
    }
};

struct device_guard : public device_guard_base
{
    void* m_ptr;

    template<typename Pointer>
    device_guard(Pointer& ptr)
#if OOMPH_ENABLE_DEVICE
    : device_guard_base(ptr.on_device(), ptr.device_id())
    , m_ptr
    {
        ptr.on_device() ? ptr.device_ptr() : ptr.get()
    }
#else
    : device_guard_base()
    , m_ptr
    {
        ptr.get()
    }
#endif
    {
    }

    void* data() const noexcept { return m_ptr; }
};

struct const_device_guard : public device_guard_base
{
    void const* m_ptr;

    template<typename Pointer>
    const_device_guard(Pointer const& ptr)
#if OOMPH_ENABLE_DEVICE
    : device_guard_base(ptr.on_device(), ptr.device_id())
    , m_ptr
    {
        ptr.on_device() ? ptr.device_ptr() : ptr.get()
    }
#else
    : device_guard_base()
    , m_ptr
    {
        ptr.get()
    }
#endif
    {
    }

    void const* data() const noexcept { return m_ptr; }
};

} // namespace oomph
