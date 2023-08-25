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
#include <oomph/util/stack_pimpl.hpp>

namespace oomph
{
namespace util
{

template<typename T, std::size_t B, std::size_t A, typename... Args>
stack_pimpl<T, B, A>
make_stack_pimpl(Args&&... args)
{
    return {T{std::forward<Args>(args)...}};
}

template<typename T, std::size_t B, std::size_t A>
stack_pimpl<T, B, A>::~stack_pimpl() = default;

template<typename T, std::size_t B, std::size_t A>
stack_pimpl<T, B, A>::stack_pimpl() noexcept = default;

template<typename T, std::size_t B, std::size_t A>
template<typename... Args>
stack_pimpl<T, B, A>::stack_pimpl(Args&&... args)
: m{std::forward<Args>(args)...}
{
}

template<typename T, std::size_t B, std::size_t A>
stack_pimpl<T, B, A>::stack_pimpl(stack_pimpl&&) noexcept = default;

template<typename T, std::size_t B, std::size_t A>
stack_pimpl<T, B, A>& stack_pimpl<T, B, A>::operator=(stack_pimpl&&) noexcept = default;

template<typename T, std::size_t B, std::size_t A>
T*
stack_pimpl<T, B, A>::operator->() noexcept
{
    return m.get();
}

template<typename T, std::size_t B, std::size_t A>
T const*
stack_pimpl<T, B, A>::operator->() const noexcept
{
    return m.get();
}

template<typename T, std::size_t B, std::size_t A>
T&
stack_pimpl<T, B, A>::operator*() noexcept
{
    return *m.get();
}

template<typename T, std::size_t B, std::size_t A>
T const&
stack_pimpl<T, B, A>::operator*() const noexcept
{
    return *m.get();
}

template<typename T, std::size_t B, std::size_t A>
T*
stack_pimpl<T, B, A>::get() noexcept
{
    return m.get();
}

template<typename T, std::size_t B, std::size_t A>
T const*
stack_pimpl<T, B, A>::get() const noexcept
{
    return m.get();
}

} // namespace util
} // namespace oomph

#define OOMPH_INSTANTIATE_STACK_PIMPL(T, B, A) template class ::oomph::util::stack_pimpl<T,B,A>;
