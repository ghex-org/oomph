/*
 * ghex-org
 *
 * Copyright (c) 2014-2025, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <nccl.h>

// TODO: Print error string and code.
#ifdef NDEBUG
#define OOMPH_CHECK_NCCL_RESULT(x)          x;
#define OOMPH_CHECK_NCCL_RESULT_NOEXCEPT(x) x;
#else
#include <string>
#include <stdexcept>
#include <iostream>
#define OOMPH_CHECK_NCCL_RESULT(x)                                                                 \
    {                                                                                              \
        ncclResult_t r = x;                                                                        \
        if (r != ncclSuccess && r != ncclInProgress)                                               \
            throw std::runtime_error("OOMPH Error: NCCL Call failed " + std::string(#x) + " = " + std::to_string(r) + " (\"" + ncclGetErrorString(r) + "\") in " + \
                                     std::string(__FILE__) + ":" + std::to_string(__LINE__));      \
    }
#define OOMPH_CHECK_NCCL_RESULT_NOEXCEPT(x)                                                        \
    {                                                                                              \
        ncclResult_t r = x;                                                                        \
        if (r != ncclSuccess && r != ncclInProgress)                                               \
        {                                                                                          \
            std::cerr << "OOMPH Error: NCCL Call failed " << std::string(#x) << " in "             \
                      << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;    \
            std::terminate();                                                                      \
        }                                                                                          \
    }
#endif
