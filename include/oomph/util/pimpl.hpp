/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <oomph/config.hpp>
#if OOMPH_USE_FAST_PIMPL
#include "./stack_pimpl.hpp"
#else
#include "./heap_pimpl.hpp"
#endif

#pragma once

namespace oomph
{
namespace util
{
#if OOMPH_USE_FAST_PIMPL
template<typename T, std::size_t S,
    std::size_t Alignment = std::alignment_of<std::max_align_t>::value>
using pimpl = stack_pimpl<T, S, Alignment>;
#else
template<typename T, std::size_t = 0, std::size_t = 0>
using pimpl = heap_pimpl<T>;
#endif
} // namespace util
} // namespace oomph
