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

namespace oomph
{

template<typename T>
class increment_guard
{
  private:
    T* m = nullptr;

  public:
    increment_guard(T& r) noexcept
    : m{&r}
    {
        ++(*m);
    }

    increment_guard(increment_guard&& other) noexcept
    : m(other.m)
    {
        other.m = nullptr;
    }

    increment_guard(increment_guard const&) = delete;
    increment_guard& operator=(increment_guard const&) = delete;
    increment_guard& operator=(increment_guard&&) = delete;

    ~increment_guard()
    {
        if (m) --(*m);
    }
};
} // namespace oomph
