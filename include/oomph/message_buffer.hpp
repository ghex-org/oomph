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

#include <oomph/detail/message_buffer.hpp>
#include <cstddef>

namespace oomph
{
template<typename T>
class message_buffer
{
  public:
    using value_type = T;

  private:
    friend class context;
    friend class communicator;

  private:
    detail::message_buffer m;
    std::size_t            m_size;

  private:
    message_buffer(detail::message_buffer&& m_, std::size_t size_)
    : m{std::move(m_)}
    , m_size{size_}
    {
    }

  public:
    message_buffer() = default;
    message_buffer(message_buffer&&) = default;
    message_buffer& operator=(message_buffer&&) = default;

  public:
    operator bool() const noexcept { return m; }

    std::size_t size() const noexcept { return m_size; }

    T*       data() noexcept { return (T*)m.m_ptr; }
    T const* data() const noexcept { return (T const*)m.m_ptr; }
    T*       begin() noexcept { return data(); }
    T const* begin() const noexcept { return data(); }
    T*       end() noexcept { return data() + size(); }
    T const* end() const noexcept { return data() + size(); }
    T const* cbegin() const noexcept { return data(); }
    T const* cend() const noexcept { return data() + size(); }

    T&       operator[](std::size_t i) noexcept { return *(data() + i); }
    T const& operator[](std::size_t i) const noexcept { return *(data() + i); }

    bool on_device() const noexcept { return m.on_device(); }

#if HWMALLOC_ENABLE_DEVICE
    T*       device_data() noexcept { return (T*)m.device_data(); }
    T const* device_data() const noexcept { return (T*)m.device_data(); }

    int device_id() const noexcept { return m.device_id(); }

    void clone_to_device() { m.clone_to_device(m_size * sizeof(T)); }
    void clone_to_host() { m.clone_to_host(m_size * sizeof(T)); }
#endif
};

} // namespace oomph
