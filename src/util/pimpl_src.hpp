/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/util/pimpl.hpp>
#if OOMPH_USE_FAST_PIMPL
#   include "./stack_pimpl_src.hpp"
//#   define OOMPH_INSTANTIATE_PIMPL(T) OOMPH_INSTANTIATE_STACK_PIMPL(T)  
#else
#   include "./heap_pimpl_src.hpp"
//#   define OOMPH_INSTANTIATE_PIMPL(T) OOMPH_INSTANTIATE_HEAP_PIMPL(T)  
#endif
