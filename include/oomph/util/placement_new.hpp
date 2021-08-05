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

#include <cstddef>
#include <cassert>
#include <memory>

#if defined(NDEBUG)
#define OOMPH_DEBUG_ARG(TYPE, NAME) TYPE
#else
#define OOMPH_DEBUG_ARG(TYPE, NAME) TYPE NAME
#endif

namespace oomph
{
namespace util
{
template<typename T, typename... Args>
inline T*
placement_new(void* buffer, OOMPH_DEBUG_ARG(std::size_t, size), Args&&... args)
{
    assert(sizeof(T) <= size);
    assert(std::align(std::alignment_of<T>::value, sizeof(T), buffer, size) == buffer);
    return new (buffer) T{std::forward<Args>(args)...};
}

template<typename T>
inline void
placement_delete(void* buffer)
{
    reinterpret_cast<T*>(buffer)->~T();
}

} // namespace util
} // namespace oomph
