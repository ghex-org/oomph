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

#include <type_traits>
#include <utility>
#include <oomph/util/placement_new.hpp>

namespace oomph
{
namespace util
{
namespace detail
{
template<std::size_t BufferSize, std::size_t Size, std::size_t BufferAlignment,
    std::size_t Alignment>
inline void
compare_size()
{
    static_assert(BufferSize >= Size, "buffer size is too small");
    static_assert(BufferAlignment >= Alignment, "buffer alignment not big enough");
}

template<std::size_t BufferSize, std::size_t Size, std::size_t BufferAlignment,
    std::size_t Alignment>
struct size_comparer
{
    inline size_comparer()
    {
        // going through one additional layer to get good error messages
        compare_size<BufferSize, Size, BufferAlignment, Alignment>();
    }
};
} // namespace detail

template<typename T, std::size_t BufferSize,
    std::size_t BufferAlignment = std::alignment_of<std::max_align_t>::value>
class stack_storage
{
  private:
    using aligned_storage = std::aligned_storage_t<BufferSize, BufferAlignment>;

  private:
    aligned_storage m_impl;
    bool            m_empty = false;

  public:
    stack_storage()
    : m_empty{true}
    {
    }

    template<typename... Args>
    stack_storage(Args&&... args)
    {
        placement_new<T>(&m_impl, BufferSize, std::forward<Args>(args)...);
    }
    stack_storage(stack_storage const&) = delete;
    stack_storage(stack_storage&& other)
    : m_empty{other.m_empty}
    {
        if (!m_empty) placement_new<T>(&m_impl, BufferSize, std::move(*other.get()));
    }
    stack_storage& operator=(stack_storage const&) = delete;
    stack_storage& operator=(stack_storage&& other)
    {
        if (m_empty && other.m_empty) { return *this; }
        else if (m_empty && !other.m_empty)
        {
            placement_new<T>(&m_impl, BufferSize, std::move(*other.get()));
            m_empty = false;
            return *this;
        }
        else if (!m_empty && other.m_empty)
        {
            placement_delete<T>(&m_impl);
            m_empty = true;
            return *this;
        }
        else
        {
            *get() = std::move(*other.get());
            return *this;
        }
    }
    ~stack_storage()
    {
        detail::size_comparer<BufferSize, sizeof(T), BufferAlignment, alignof(T)> s{};
        if (!m_empty) placement_delete<T>(&m_impl);
    }

    T* get() noexcept
    {
        assert(!m_empty);
        return reinterpret_cast<T*>(&m_impl);
    }

    T const* get() const noexcept
    {
        assert(!m_empty);
        return reinterpret_cast<T const*>(&m_impl);
    }

    //T release()
    //{
    //    T t{std::move(*get())};
    //    placement_delete<T>(&m_impl);
    //    m_empty = true;
    //    return std::move(t);
    //}
};

} // namespace util
} // namespace oomph
