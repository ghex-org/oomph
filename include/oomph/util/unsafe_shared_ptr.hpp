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

#include <utility>
#include <memory>
#include <cassert>

namespace oomph
{
namespace util
{

template<typename T>
class enable_shared_from_this;

namespace detail
{

template<typename T>
struct control_block
{
    std::size_t m_ref_count = 1ul;
    T*          m_ptr = nullptr;

    virtual void free() = 0;
};

template<typename T, typename Allocator>
struct control_block_impl : public control_block<T>
{
    using this_type = control_block_impl<T, Allocator>;
    using base_type = control_block<T>;
    using alloc_t = typename std::allocator_traits<Allocator>::template rebind_alloc<this_type>;
    using traits = std::allocator_traits<alloc_t>;

    alloc_t m_alloc;
    T       m_t;

    template<typename Alloc, typename... Args>
    control_block_impl(Alloc const a, Args&&... args)
    : base_type()
    , m_alloc{a}
    , m_t{std::forward<Args>(args)...}
    {
        this->m_ptr = &m_t;
        set_shared_from_this<T>();
    }

    void free() override final
    {
        auto a = m_alloc;
        m_alloc.~alloc_t();
        m_t.~T();
        traits::deallocate(a, this, 1);
    }

    template<typename D,
        std::enable_if_t<!std::is_base_of<enable_shared_from_this<D>, D>::value, bool> = true>
    void set_shared_from_this()
    {
    }

    template<typename D,
        std::enable_if_t<std::is_base_of<enable_shared_from_this<D>, D>::value, bool> = true>
    void set_shared_from_this()
    {
        m_t._shared_from_this_cb = this;
    }
};

} // namespace detail

template<typename T>
class unsafe_shared_ptr
{
    template<typename D>
    friend class enable_shared_from_this;

  private:
    using block_t = detail::control_block<T>;

  public:
    template<typename Alloc>
    static constexpr std::size_t allocation_size()
    {
        return sizeof(detail::control_block_impl<T, Alloc>);
    }

  private:
    block_t* m = nullptr;

  private:
    unsafe_shared_ptr(block_t* m_) noexcept
    : m{m_}
    {
        if (m) ++m->m_ref_count;
    }

  public:
    template<typename Alloc, typename... Args>
    unsafe_shared_ptr(Alloc const& alloc, Args&&... args)
    {
        using block_impl_t = detail::control_block_impl<T, Alloc>;
        using alloc_t = typename block_impl_t::alloc_t;
        using traits = std::allocator_traits<alloc_t>;

        alloc_t a(alloc);
        m = traits::allocate(a, 1);
        ::new (m) block_impl_t(a, std::forward<Args>(args)...);
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

    ~unsafe_shared_ptr() { destroy(); }

    operator bool() const noexcept { return (bool)m; }

    T* get() const noexcept { return m->m_ptr; }

    T* operator->() const noexcept { return m->m_ptr; }

    T& operator*() const noexcept { return *(m->m_ptr); }

    std::size_t use_count() const noexcept { return m ? m->m_ref_count : 0ul; }

  private:
    void destroy() noexcept
    {
        if (!m) return;
        if (--m->m_ref_count == 0) m->free();
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

template<typename D>
class enable_shared_from_this
{
    template<typename T, typename Alloc>
    friend struct detail::control_block_impl;

  private:
    detail::control_block<D>* _shared_from_this_cb = nullptr;

  public:
    enable_shared_from_this() noexcept {}
    enable_shared_from_this(enable_shared_from_this const&) noexcept {}

  protected:
    enable_shared_from_this& operator=(enable_shared_from_this const&) noexcept
    {
        _shared_from_this_cb = nullptr;
    }

  public:
    unsafe_shared_ptr<D> shared_from_this()
    {
        assert(((bool)_shared_from_this_cb) && "not created by a unsafe_shared_ptr");
        return {_shared_from_this_cb};
    }

  private:
    D* derived() noexcept { return static_cast<D*>(this); }
};

} // namespace util
} // namespace oomph
