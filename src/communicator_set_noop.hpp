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
#include <../communicator_set.hpp>

namespace oomph
{

struct communicator_set::impl
{
    void insert(context_impl const*, communicator_impl*) {}

    void erase(context_impl const*, communicator_impl*) {}

    void erase(context_impl const*) {}

    void progress(context_impl const*) {}
};

} // namespace oomph
