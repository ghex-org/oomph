/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "../thread_id.hpp"

namespace oomph
{
namespace
{
std::uintptr_t*
alloc_tid_m()
{
    auto ptr = new std::uintptr_t{};
    *ptr = (std::uintptr_t)ptr;
    return ptr;
}
} // namespace

thread_id::thread_id()
: m{alloc_tid_m()}
{
}

thread_id::~thread_id() { delete m; }

thread_id const&
tid()
{
    static thread_local thread_id id;
    return id;
}
} // namespace oomph
