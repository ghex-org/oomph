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

namespace oomph
{
struct nccl_request
{
    // TODO: Ready when group has completed? Check stream or event?
    bool is_ready() { return true; }
    // TODO: No cancellation with NCCL?
    bool cancel() { return false; }
};
} // namespace oomph
