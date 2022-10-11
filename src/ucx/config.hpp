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

#include <mutex>

#include <oomph/config.hpp>

// paths relative to backend
#include <error.hpp>
#ifdef OOMPH_UCX_USE_PMI
#include <address_db_pmi.hpp>
#else
#include <address_db_mpi.hpp>
#endif

#ifdef OOMPH_UCX_USE_SPIN_LOCK
#include <pthread_spin_mutex.hpp>
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
