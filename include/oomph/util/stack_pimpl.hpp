/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
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
  private:
    util::stack_storage<T, BufferSize, Alignment> m;

  public:
    ~stack_pimpl();
    stack_pimpl() noexcept;
    template<typename... Args>
    stack_pimpl(Args&&... args);
    stack_pimpl(stack_pimpl const&) = delete;
    stack_pimpl(stack_pimpl&&) noexcept;
    stack_pimpl& operator=(stack_pimpl const&) = delete;
    stack_pimpl& operator=(stack_pimpl&&) noexcept;

    T*       operator->() noexcept;
    T const* operator->() const noexcept;
    T&       operator*() noexcept;
    T const& operator*() const noexcept;
    T*       get() noexcept;
    T const* get() const noexcept;
};

template<typename T, std::size_t BufferSize,
    std::size_t Alignment = std::alignment_of<std::max_align_t>::value, typename... Args>
stack_pimpl<T, BufferSize, Alignment> make_stack_pimpl(Args&&... args);

} // namespace util
} // namespace oomph
