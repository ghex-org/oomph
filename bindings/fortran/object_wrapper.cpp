/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <fortran/oomph_defs.hpp>
#include <fortran/object_wrapper.hpp>

extern "C" void
oomph_obj_free(oomph::fort::obj_wrapper** wrapper_ref)
{
    auto wrapper = *wrapper_ref;

    /* clear the fortran-side variable */
    *wrapper_ref = nullptr;
    delete wrapper;
}
