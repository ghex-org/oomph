/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/util/stack_storage.hpp>

namespace oomph
{
namespace util
{
template<typename T, std::size_t BufferSize,
    std::size_t Alignment = std::alignment_of<std::max_align_t>::value>
class stack_pimpl
{
    util::stack_storage<T, BufferSize, Alignment> m;

  public:
    stack_pimpl() = default;
    template<typename... Args>
    stack_pimpl(Args&&... args)
    : m(std::forward<Args>(args)...)
    {
    }
    stack_pimpl(stack_pimpl const&) = delete;
    stack_pimpl(stack_pimpl&&) = default;
    stack_pimpl& operator=(stack_pimpl const&) = delete;
    stack_pimpl& operator=(stack_pimpl&&) = default;

    T*       operator->() noexcept { return m.get(); }
    T const* operator->() const noexcept { return m.get(); }
    T&       operator*() noexcept { return *m.get(); }
    T const& operator*() const noexcept { return *m.get(); }

    //T release() { return m.release(); }
};

} // namespace util
} // namespace oomph
