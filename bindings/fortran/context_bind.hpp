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
#include <oomph/context.hpp>

namespace oomph
{
namespace fort
{
context& get_context();
    using context_type = oomph::context;
    using communicator_type = oomph::communicator;
} // namespace fort
} // namespace oomph
