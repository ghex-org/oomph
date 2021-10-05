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

#include <oomph/util/mpi_error.hpp>
#include <oomph/communicator.hpp>
#include <set>
#include <mutex>

namespace oomph
{
class lock_cache
{
  public:
    using rank_type = communicator::rank_type;

  private:
    MPI_Win             m_win;
    std::set<rank_type> m_ranks;
    std::mutex          m_mutex;

  public:
    lock_cache(MPI_Win win) noexcept
    : m_win(win)
    {
    }

    lock_cache(lock_cache const&) = delete;

    ~lock_cache()
    {
        for (auto r : m_ranks) MPI_Win_unlock(r, m_win);
    }

    void lock(rank_type r)
    {
        std::lock_guard<std::mutex> l(m_mutex);

        auto it = m_ranks.find(r);
        if (it == m_ranks.end())
        {
            m_ranks.insert(r);
            OOMPH_CHECK_MPI_RESULT(MPI_Win_lock(MPI_LOCK_SHARED, r, 0, m_win));
        }
    }
};

} // namespace oomph
