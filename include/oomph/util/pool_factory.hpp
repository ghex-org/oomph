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
#include <oomph/util/pool_allocator.hpp>
#include <oomph/util/unsafe_shared_ptr.hpp>

namespace oomph
{
namespace util
{

template<class T>
struct pool_factory
{
  public:
    using value_type = T;
    using ptr_type = unsafe_shared_ptr<value_type>;

  private:
    using allocator_type = pool_allocator<char>;
    using pool_type = typename allocator_type::pool_type;

    std::unique_ptr<pool_type> m_pool;

  public:
    pool_factory()
    : m_pool{std::make_unique<pool_type>(ptr_type::template allocation_size<allocator_type>())}
    {
    }

    pool_factory(pool_factory const&) = delete;
    pool_factory(pool_factory&&) = default;
    pool_factory& operator=(pool_factory const&) = delete;
    pool_factory& operator=(pool_factory&&) = default;

    template<typename... Args>
    ptr_type make(Args&&... args)
    {
        return oomph::util::allocate_shared<value_type>(allocator_type(m_pool.get()),
            std::forward<Args>(args)...);
    }
};

} // namespace util
} // namespace oomph
