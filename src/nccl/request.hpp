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

#include <cuda_runtime.h>

#include "./cuda_error.hpp"

namespace oomph
{
struct nccl_request
{
    // TODO: Ready when group has completed? Check stream or event?
    bool is_ready() {
        std::cerr << "checking if request is ready\n";
        cudaError_t res = cudaEventQuery(m_event);
        std::cerr << "request " << m_event << " is in state " << res << "\n";
        if (res == cudaSuccess) {
        	return true;
        } else if (res == cudaErrorNotReady) {
        	return false;
        } else {
        	OOMPH_CHECK_CUDA_RESULT(res);
                return false;
        }
    }
    // TODO: No cancellation with NCCL?
    bool cancel() { return false; }

    // TODO: Use wrapper class
    cudaEvent_t m_event;
};
} // namespace oomph
