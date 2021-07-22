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
    heap_pimpl();
    template<typename... Args>
    heap_pimpl(Args&&...);
    heap_pimpl(heap_pimpl const&) = delete;
    heap_pimpl(heap_pimpl&&);
    heap_pimpl& operator=(heap_pimpl const&) = delete;
    heap_pimpl& operator=(heap_pimpl&&);
    ~heap_pimpl();

    T*       operator->();
    T const* operator->() const;
    T&       operator*();
    T const& operator*() const;

    T release();
};

} // namespace util
} // namespace oomph
