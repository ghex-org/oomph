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
#include <oomph/util/heap_pimpl.hpp>

namespace oomph
{
namespace util
{

template<typename T, typename... Args>
heap_pimpl<T>
make_heap_pimpl(Args&&... args)
{
    return {new T{std::forward<Args>(args)...}};
}

template<typename T>
heap_pimpl<T>::~heap_pimpl() = default;

template<typename T>
heap_pimpl<T>::heap_pimpl() noexcept = default;

template<typename T>
heap_pimpl<T>::heap_pimpl(T* ptr) noexcept
: m{ptr}
{
}

template<typename T>
template<typename... Args>
heap_pimpl<T>::heap_pimpl(Args&&... args)
: m{new T{std::forward<Args>(args)...}}
{
}

template<typename T>
heap_pimpl<T>::heap_pimpl(heap_pimpl&&) noexcept = default;

template<typename T>
heap_pimpl<T>& heap_pimpl<T>::operator=(heap_pimpl&&) noexcept = default;

template<typename T>
T*
heap_pimpl<T>::operator->() noexcept
{
    return m.get();
}

template<typename T>
T const*
heap_pimpl<T>::operator->() const noexcept
{
    return m.get();
}

template<typename T>
T&
heap_pimpl<T>::operator*() noexcept
{
    return *m.get();
}

template<typename T>
T const&
heap_pimpl<T>::operator*() const noexcept
{
    return *m.get();
}

template<typename T>
T*
heap_pimpl<T>::get() noexcept
{
    return m.get();
}

template<typename T>
T const*
heap_pimpl<T>::get() const noexcept
{
    return m.get();
}

} // namespace util
} // namespace oomph

#define OOMPH_INSTANTIATE_HEAP_PIMPL(T) template class ::oomph::util::heap_pimpl<T>;
