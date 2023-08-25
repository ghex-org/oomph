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

#include <memory>

namespace oomph
{
namespace util
{
template<typename T>
class heap_pimpl
{
  private:
    std::unique_ptr<T> m;

  public:
    ~heap_pimpl();
    heap_pimpl() noexcept;
    heap_pimpl(T* ptr) noexcept;
    template<typename... Args>
    heap_pimpl(Args&&... args);
    heap_pimpl(heap_pimpl const&) = delete;
    heap_pimpl(heap_pimpl&&) noexcept;
    heap_pimpl& operator=(heap_pimpl const&) = delete;
    heap_pimpl& operator=(heap_pimpl&&) noexcept;

    T*       operator->() noexcept;
    T const* operator->() const noexcept;
    T&       operator*() noexcept;
    T const& operator*() const noexcept;
    T*       get() noexcept;
    T const* get() const noexcept;
};

template<typename T, typename... Args>
heap_pimpl<T> make_heap_pimpl(Args&&... args);

} // namespace util
} // namespace oomph
