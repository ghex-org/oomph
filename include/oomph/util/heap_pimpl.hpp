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
    heap_pimpl() = default;
    template<typename... Args>
    heap_pimpl(Args&&... args)
    : m{new T{std::forward<Args>(args)...}}
    {
    }
    heap_pimpl(heap_pimpl const&) = delete;
    heap_pimpl(heap_pimpl&&) = default;
    heap_pimpl& operator=(heap_pimpl const&) = delete;
    heap_pimpl& operator=(heap_pimpl&&) = default;

    T*       operator->() noexcept { return m.get(); }
    T const* operator->() const noexcept { return m.get(); }
    T&       operator*() noexcept { return *m.get(); }
    T const& operator*() const noexcept { return *m.get(); }

    T*       get() noexcept { return m.get(); }
    T const* get() const noexcept { return m.get(); }

    //T release()
    //{
    //    auto ptr = m.release();
    //    T    t{std::move(*ptr)};
    //    delete ptr;
    //    return std::move(t);
    //}
};

} // namespace util
} // namespace oomph
