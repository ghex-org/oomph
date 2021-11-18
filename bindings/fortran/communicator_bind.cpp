/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <fortran/object_wrapper.hpp>
#include <fortran/context_bind.hpp>

using namespace oomph::fort;

extern "C"
void* oomph_get_communicator()
{
    return new obj_wrapper(get_context().get_communicator());
}

