#pragma once

#include <memory>
#include <typeinfo>
#include <type_traits>

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
};

template<typename Func, typename R, typename... Args>
struct unique_function_impl : unique_function<R, Args...>
{
    Func func;

    template<typename F>
    unique_function_impl(F&& f)
    : func{std::forward<F>(f)}
    {
    }

    virtual R invoke(Args&&... args) final override { return func(std::forward<Args>(args)...); }
};

// specialization for void function
template<typename Func, typename... Args>
struct unique_function_impl<Func, void, Args...> : unique_function<void, Args...>
{
    Func func;

    template<typename F>
    unique_function_impl(F&& f)
    : func{std::forward<F>(f)}
    {
    }

    virtual void invoke(Args&&... args) final override { func(std::forward<Args>(args)...); }
};

} // namespace detail

template<typename R, typename... Args>
class unique_function<R(Args...)>
{
  private: // members
    using interface_t = detail::unique_function<R, Args...>;
    using u_ptr_t = std::unique_ptr<interface_t>;
    template<typename F>
    using result_t = std::result_of_t<F&(Args...)>;
    template<typename F>
    using concrete_t = detail::unique_function_impl<std::decay_t<F>, R, Args...>;

    u_ptr_t impl;

  public: // ctors
    unique_function() = default;
    unique_function(unique_function const&) = delete;
    unique_function& operator=(unique_function const&) = delete;
    unique_function(unique_function&& other) = default;
    unique_function& operator=(unique_function&& other) = default;

    template<typename F,
        // F can be invoked with Args and return type can be converted to R
        typename = decltype((R)(std::declval<result_t<F>>())),
        // F is not a unique_function
        std::enable_if_t<!std::is_same<std::decay_t<F>, unique_function>::value, int>* = nullptr>
    unique_function(F&& f)
    : impl{std::make_unique<concrete_t<F>>(std::forward<F>(f))}
    {
    }

  public: // member functions
    R operator()(Args... args) const { return impl->invoke(std::forward<Args>(args)...); }

    interface_t* release() noexcept { return impl.release(); }
};

} // namespace util
} // namespace oomph
