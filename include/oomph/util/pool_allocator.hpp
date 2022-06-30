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

#include <cassert>
#include <new>
#include <boost/pool/pool.hpp>

namespace oomph
{
namespace util
{

template<class T>
struct pool_allocator
{
    using value_type = T;
    using pool_type = boost::pool<boost::default_user_allocator_malloc_free>;

    pool_type* _p;

    constexpr pool_allocator(pool_type* p) noexcept
    : _p{p}
    {
    }

    template<class U>
    constexpr pool_allocator(const pool_allocator<U>& other) noexcept
    : _p{other._p}
    {
    }

#ifdef NDEBUG
    [[nodiscard]] T* allocate(std::size_t)
#else
    [[nodiscard]] T* allocate(std::size_t n)
#endif
    {
        assert(_p->get_requested_size() >= sizeof(T) * n);
        if (auto ptr = static_cast<T*>(_p->malloc())) return ptr;
        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t /*n*/) noexcept { _p->free(p); }
};

template<class T, class U>
bool
operator==(const pool_allocator<T>&, const pool_allocator<U>&)
{
    return true;
}
template<class T, class U>
bool
operator!=(const pool_allocator<T>&, const pool_allocator<U>&)
{
    return false;
}

} // namespace util
} // namespace oomph
