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

#include <cstdlib>
#include <new>
#include <limits>
#include <iostream>

template<class T>
struct reporting_allocator
{
    using value_type = T;

    reporting_allocator() noexcept {}

    template<class U>
    constexpr reporting_allocator(const reporting_allocator<U>&) noexcept
    {
    }

    [[nodiscard]] T* allocate(std::size_t n)
    {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();

        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
        {
            report(p, n);
            return p;
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        report(p, n, 0);
        std::free(p);
    }

  private:
    void report(T* p, std::size_t n, bool alloc = true) const
    {
        std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T) * n << " bytes at " << std::hex
                  << std::showbase << reinterpret_cast<void*>(p) << std::dec << '\n';
    }
};

template<class T, class U>
bool
operator==(const reporting_allocator<T>&, const reporting_allocator<U>&)
{
    return true;
}
template<class T, class U>
bool
operator!=(const reporting_allocator<T>&, const reporting_allocator<U>&)
{
    return false;
}
