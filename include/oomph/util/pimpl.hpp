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

#include <utility>

#include <oomph/config.hpp>
#if OOMPH_USE_FAST_PIMPL
#include "./stack_pimpl.hpp"
#else
#include "./heap_pimpl.hpp"
#endif

namespace oomph
{
namespace util
{
#if OOMPH_USE_FAST_PIMPL

template<typename T, std::size_t S,
    std::size_t Alignment = std::alignment_of<std::max_align_t>::value>
using pimpl = stack_pimpl<T, S, Alignment>;

template<typename T, std::size_t BufferSize,
    std::size_t Alignment = std::alignment_of<std::max_align_t>::value, typename... Args>
pimpl<T, BufferSize, Alignment>
make_pimpl(Args&&... args)
{
    return make_stack_pimpl(std::forward<Args>(args)...);
}

#else

template<typename T, std::size_t = 0, std::size_t = 0>
using pimpl = heap_pimpl<T>;

template<typename T, std::size_t = 0, std::size_t = 0, typename... Args>
pimpl<T, 0, 0>
make_pimpl(Args&&... args)
{
    return make_heap_pimpl(std::forward<Args>(args)...);
}

#endif
} // namespace util
} // namespace oomph
