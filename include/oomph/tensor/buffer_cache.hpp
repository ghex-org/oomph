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

#include <oomph/tensor/detail/buffer_cache.hpp>

namespace oomph
{
namespace tensor
{
template<typename T>
struct buffer_cache
{
    std::shared_ptr<detail::buffer_cache<T, communicator*>> m_cache;

    buffer_cache()
    : m_cache{std::make_shared<detail::buffer_cache<T, communicator*>>()}
    {
    }
    buffer_cache(buffer_cache const&) = default;
    buffer_cache& operator=(buffer_cache const&) = default;
};
} // namespace tensor
} // namespace oomph
