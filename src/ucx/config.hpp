/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma once

#include <oomph/config.hpp>
#include "./error.hpp"

#ifdef OOMPH_UCX_USE_PMI
#include "./address_db_pmi.hpp"
#else
#include "./address_db_mpi.hpp"
#endif

#include <mutex>
#ifdef OOMPH_UCX_USE_SPIN_LOCK
#include "./pthread_spin_mutex.hpp"
namespace oomph
{
using ucx_mutex = pthread_spin::mutex;
}
#else
namespace oomph
{
using ucx_mutex = std::mutex;
}
#endif
namespace oomph
{
using ucx_lock = std::lock_guard<ucx_mutex>;
}
