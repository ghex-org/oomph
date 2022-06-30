/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma once

// ------------------------------------------------------------------
// This section exists to make interoperabily/sharing of code
// between OOMPH/GHEX and HPX easier
#if __has_include(<hpx/config/parcelport_defines.hpp>)
#include <hpx/config/parcelport_defines.hpp>
#elif __has_include("oomph_libfabric_defines.hpp")
#include "oomph_libfabric_defines.hpp"
#endif
