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

#include <memory>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <variant>

namespace oomph
{
namespace util
{
template<typename Signature>
class unique_function;

namespace detail
{
template<typename R, typename... Args>
struct unique_function
{
    virtual R invoke(Args&&... args) = 0;
    virtual ~unique_function(){};

    virtual void move_construct(void*) = 0;
};

template<typename Func, typename R, typename... Args>
struct unique_function_impl : unique_function<R, Args...>
{
    using this_type = unique_function_impl<Func, R, Args...>;

    Func func;

    template<typename F>
    unique_function_impl(F&& f)
    : func{std::forward<F>(f)}
    {
    }

    virtual R invoke(Args&&... args) final override { return func(std::forward<Args>(args)...); }

    virtual void move_construct(void* addr) final override
    {
        ::new (addr) this_type{std::move(func)};
    }
};

// specialization for void function
template<typename Func, typename... Args>
struct unique_function_impl<Func, void, Args...> : unique_function<void, Args...>
{
    using this_type = unique_function_impl<Func, void, Args...>;

    Func func;

    template<typename F>
    unique_function_impl(F&& f)
    : func{std::forward<F>(f)}
    {
    }

    virtual void invoke(Args&&... args) final override { func(std::forward<Args>(args)...); }

    virtual void move_construct(void* addr) final override
    {
        ::new (addr) this_type{std::move(func)};
    }
};

} // namespace detail

template<typename R, typename... Args>
class unique_function<R(Args...)>
{
  private: // members
    using interface_t = detail::unique_function<R, Args...>;
    template<typename F>
    using result_t = std::result_of_t<F&(Args...)>;
    template<typename F>
    using concrete_t = detail::unique_function_impl<std::decay_t<F>, R, Args...>;
    static constexpr std::size_t sbo_size = 128;
    using buffer_t = std::aligned_storage_t<sbo_size, std::alignment_of<std::max_align_t>::value>;

    // variant holds 3 alternatives:
    // - empty state
    // - heap allocated function objects
    // - stack buffer for small function objects (sbo)
    std::variant<std::monostate, interface_t*, buffer_t> holder;

  public: // ctors
    // construct empty
    unique_function() noexcept = default;

    // deleted copy ctors
    unique_function(unique_function const&) = delete;
    unique_function& operator=(unique_function const&) = delete;

    unique_function(unique_function&& other) noexcept
    : holder{std::move(other.holder)}
    {
        // explicitely move if function is stored in stack buffer
        if (holder.index() == 2) other.get_from_buffer()->move_construct(&std::get<2>(holder));
        // reset other to empty state
        other.holder = std::monostate{};
    }

    unique_function& operator=(unique_function&& other) noexcept
    {
        holder = std::move(other.holder);
        // explicitely move if function is stored in stack buffer
        if (holder.index() == 2) other.get_from_buffer()->move_construct(&std::get<2>(holder));
        // reset other to empty state
        other.holder = std::monostate{};
        return *this;
    }

    ~unique_function()
    {
        // delete from heap
        if (holder.index() == 1) delete std::get<1>(holder);
        // delete from stack buffer
        if (holder.index() == 2) std::destroy_at(get_from_buffer());
    }

    template<typename F,
        // F can be invoked with Args and return type can be converted to R
        typename = decltype((R)(std::declval<result_t<F>>())),
        // F is not a unique_function and bigger than the stack buffer
        std::enable_if_t<!std::is_same<std::decay_t<F>, unique_function>::value &&
                             (sizeof(std::decay_t<F>) > sbo_size),
            bool> = true>
    unique_function(F&& f)
    : holder{std::in_place_type_t<interface_t*>{}, new concrete_t<F>(std::forward<F>(f))}
    {
    }

    template<typename F,
        // F can be invoked with Args and return type can be converted to R
        typename = decltype((R)(std::declval<result_t<F>>())),
        // F is not a unique_function and smaller of equal than the stack buffer
        std::enable_if_t<!std::is_same<std::decay_t<F>, unique_function>::value &&
                             (sizeof(std::decay_t<F>) <= sbo_size),
            bool> = true>
    unique_function(F&& f)
    : holder{std::in_place_type_t<buffer_t>{}}
    {
        // in-place construct
        ::new (&std::get<2>(holder)) concrete_t<F>(std::forward<F>(f));
    }

  public: // member functions
    R operator()(Args... args) const
    {
        if (holder.index() == 2) return get_from_buffer()->invoke(std::forward<Args>(args)...);
        return std::get<1>(holder)->invoke(std::forward<Args>(args)...);
    }

  private: // helper functions
    interface_t* get_from_buffer() const noexcept
    {
        return std::launder(
            reinterpret_cast<interface_t*>(&const_cast<buffer_t&>(std::get<2>(holder))));
    }
};

} // namespace util
} // namespace oomph
