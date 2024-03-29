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

// paths relative to backend
#include <error.hpp>

namespace oomph
{
struct handle
{
    void*       m_ptr;
    std::size_t m_size;
};

} // namespace oomph
