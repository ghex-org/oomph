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
    : func{std::move(f)}
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
    : func{std::move(f)}
    {
    }

    virtual void invoke(Args&&... args) final override { func(std::forward<Args>(args)...); }

    virtual void move_construct(void* addr) final override
    {
        ::new (addr) this_type{std::move(func)};
    }
};

} // namespace detail

// a function object wrapper a la std::function but for move-only types
// which uses small buffer optimization
template<typename R, typename... Args>
class unique_function<R(Args...)>
{
  private: // member types
    // abstract base class
    using interface_t = detail::unique_function<R, Args...>;

    // buffer size and type
    static constexpr std::size_t sbo_size = 128;
    using buffer_t = std::aligned_storage_t<sbo_size, std::alignment_of<std::max_align_t>::value>;

    // variant holds 3 alternatives:
    // - empty state
    // - heap allocated function objects
    // - stack buffer for small function objects (sbo)
    using holder_t = std::variant<std::monostate, interface_t*, buffer_t>;

  private: // members
    holder_t holder;

  private: // helper templates for type inspection
    // return type
    template<typename F>
    using result_t = std::result_of_t<F&(Args...)>;
    // concrete type for allocation
    template<typename F>
    using concrete_t = detail::unique_function_impl<std::decay_t<F>, R, Args...>;
    // F can be invoked with Args and return type can be converted to R
    template<typename F>
    using has_signature_t = decltype((R)(std::declval<result_t<F>>()));
    // is already a unique function
    template<typename F>
    using is_unique_function_t = std::is_same<std::decay_t<F>, unique_function>;
    // differentiate small and large function objects
    template<typename F>
    using enable_if_large_function_t =
        std::enable_if_t<!is_unique_function_t<F>::value && (sizeof(std::decay_t<F>) > sbo_size),
            bool>;
    template<typename F>
    using enable_if_small_function_t =
        std::enable_if_t<!is_unique_function_t<F>::value && (sizeof(std::decay_t<F>) <= sbo_size),
            bool>;

  public: // ctors
    // construct empty
    unique_function() noexcept = default;

    // deleted copy ctors
    unique_function(unique_function const&) = delete;
    unique_function& operator=(unique_function const&) = delete;


    // construct from large function
    template<typename F, typename = has_signature_t<F>, enable_if_large_function_t<F> = true>
    unique_function(F&& f)
    : holder{std::in_place_type_t<interface_t*>{}, new concrete_t<F>(std::move(f))}
    {
        static_assert(std::is_rvalue_reference_v<F&&>, "argument is not an r-value reference");
    }

    // construct from small function
    template<typename F, typename = has_signature_t<F>, enable_if_small_function_t<F> = true>
    unique_function(F&& f)
    : holder{std::in_place_type_t<buffer_t>{}}
    {
        static_assert(std::is_rvalue_reference_v<F&&>, "argument is not an r-value reference");
        ::new (&std::get<2>(holder)) concrete_t<F>(std::forward<F>(f));
    }

    // move construct from unique_function
    unique_function(unique_function&& other) noexcept
    : holder{std::move(other.holder)}
    {
        move_construct(other.holder);
    }

    // move assign from unique_function
    unique_function& operator=(unique_function&& other) noexcept
    {
        destroy();
        holder = std::move(other.holder);
        move_construct(other.holder);
        return *this;
    }

    // move assign from large function
    template<typename F, typename = has_signature_t<F>, enable_if_large_function_t<F> = true>
    unique_function& operator=(F&& f) noexcept
    {
        static_assert(std::is_rvalue_reference_v<F&&>, "argument is not an r-value reference");
        destroy();
        holder.template emplace<interface_t*>(new concrete_t<F>(std::move(f)));
        return *this;
    }

    // move assign from small function
    template<typename F, typename = has_signature_t<F>, enable_if_small_function_t<F> = true>
    unique_function& operator=(F&& f) noexcept
    {
        static_assert(std::is_rvalue_reference_v<F&&>, "argument is not an r-value reference");
        destroy();
        holder.template emplace<buffer_t>();
        ::new (&std::get<2>(holder)) concrete_t<F>(std::forward<F>(f));
        return *this;
    }

    ~unique_function() { destroy(); }

  public: // member functions
    R operator()(Args... args) const { return get()->invoke(std::forward<Args>(args)...); }

    operator bool() const noexcept { return (holder.index() != 0); }

  private: // helper functions
    static interface_t* get_from_buffer(holder_t const& h) noexcept
    {
        return std::launder(reinterpret_cast<interface_t*>(&const_cast<buffer_t&>(std::get<2>(h))));
    }

    static interface_t* get_from_buffer(holder_t& h) noexcept
    {
        return std::launder(reinterpret_cast<interface_t*>(&std::get<2>(h)));
    }

    interface_t* get() const noexcept
    {
        return (holder.index() == 2) ? get_from_buffer(holder) : std::get<1>(holder);
    }

    void destroy()
    {
        // delete from heap
        if (holder.index() == 1) delete std::get<1>(holder);
        // delete from stack buffer
        if (holder.index() == 2) std::destroy_at(get_from_buffer(holder));
    }

    void move_construct(holder_t& other_holder)
    {
        // explicitly move if function is stored in stack buffer
        if (other_holder.index() == 2)
        {
            interface_t* ptr = get_from_buffer(other_holder);
            ptr->move_construct(&std::get<2>(holder));
            std::destroy_at(ptr);
        }
        // reset to empty state
        other_holder = std::monostate{};
    }
};

} // namespace util
} // namespace oomph
