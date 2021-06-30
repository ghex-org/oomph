#pragma once

#include <utility>

namespace oomph
{
namespace util
{
template<typename T>
template<typename... Args>
heap_pimpl<T>::heap_pimpl(Args&&... args)
: m{new T{std::forward<Args>(args)...}}
{
}

template<typename T>
heap_pimpl<T>::heap_pimpl(heap_pimpl&&) = default;

template<typename T>
heap_pimpl<T>& heap_pimpl<T>::operator=(heap_pimpl&&) = default;

template<typename T>
heap_pimpl<T>::~heap_pimpl() = default;

template<typename T>
T*
heap_pimpl<T>::operator->()
{
    return m.get();
}

template<typename T>
T const*
heap_pimpl<T>::operator->() const
{
    return m.get();
}

template<typename T>
T&
heap_pimpl<T>::operator*()
{
    return *m.get();
}

template<typename T>
T const&
heap_pimpl<T>::operator*() const
{
    return *m.get();
}

} // namespace util
} // namespace oomph
