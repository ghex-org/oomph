/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma once

#include <oomph/util/unique_function.hpp>
#include <utility>
#include <memory>

namespace oomph
{
namespace util
{

namespace detail
{

template<typename T>
struct control_block
{
    using this_type = control_block<T>;
    using deleter_type = oomph::util::unique_function<void(this_type*), 16>;

    deleter_type m_deleter;
    T            m_t;
    std::size_t  m_ref_count = 1ul;

    template<typename Deleter, typename... Args>
    control_block(Deleter&& d, Args&&... args)
    : m_deleter{std::forward<Deleter>(d)}
    , m_t{std::forward<Args>(args)...}
    {}

};

} // namespace detail

template<typename T>
class unsafe_shared_ptr
{
  private:
    using block_t = detail::control_block<T>;

  private:
    block_t* m = nullptr;

  public:
    template<typename Alloc, typename... Args>
    unsafe_shared_ptr(Alloc const& alloc, Args&&... args)
    {
        using alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<block_t>;
        using traits = std::allocator_traits<alloc_t>;

        alloc_t a(alloc);
        m = traits::allocate(a, 1);

        traits::construct(a, //m, T{std::forward<Args>(args)...}, 0ul,
            m,
            [a](block_t* ptr) mutable
            {
                ptr->~block_t();
                traits::deallocate(a, ptr, 1);
            }, std::forward<Args>(args)...);
    }

  public:
    unsafe_shared_ptr() noexcept = default;

    unsafe_shared_ptr(unsafe_shared_ptr const& other) noexcept
    : m{other.m}
    {
        if (m) ++m->m_ref_count;
    }

    unsafe_shared_ptr(unsafe_shared_ptr&& other) noexcept
    : m{std::exchange(other.m, nullptr)}
    {
    }

    unsafe_shared_ptr& operator=(unsafe_shared_ptr const& other) noexcept
    {
        destroy();
        m = other.m;
        if (m) ++m->m_ref_count;
        return *this;
    }

    unsafe_shared_ptr& operator=(unsafe_shared_ptr&& other) noexcept
    {
        destroy();
        m = std::exchange(other.m, nullptr);
        return *this;
    }

    ~unsafe_shared_ptr()
    {
        destroy();
    }

    operator bool() const noexcept { return (bool)m; }
    
    T* get() const noexcept { return &(m->m_t); }

    T* operator->() const noexcept { return &(m->m_t); }

    T& operator*() const noexcept { return m->m_t; }

    std::size_t use_count() const noexcept { return m ? m->m_ref_count : 0ul; }

  private:
    void destroy() noexcept
    {
        if (!m) return;
        if (--m->m_ref_count == 0)
        {
            auto d = std::move(m->m_deleter);
            d(m);
        }
        m = nullptr;
    }
};

template<typename T, typename... Args>
unsafe_shared_ptr<T>
make_shared(Args&&... args)
{
    return {std::allocator<char>{}, std::forward<Args>(args)...};
}

template<typename T, typename Alloc, typename... Args>
unsafe_shared_ptr<T>
allocate_shared(Alloc const& alloc, Args&&... args)
{
    return {alloc, std::forward<Args>(args)...};
}


} // namespace util
} // namespace oomph
