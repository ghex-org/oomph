/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma once

#include <vector>
#include <unordered_set>
#include <oomph/util/mpi_error.hpp>

namespace oomph
{
/** @brief Class representing node (shared memory) topology. */
class rank_topology
{
  public: // member types
    using set_type = std::unordered_set<int>;
    using size_type = set_type::size_type;

  private: // members
    MPI_Comm                m_comm;
    MPI_Comm                m_shared_comm;
    int                     m_rank;
    std::unordered_set<int> m_rank_set;

  public: // ctors
    /** @brief construct from MPI communicator */
    rank_topology(MPI_Comm comm);
    rank_topology(const rank_topology&) = default;
    rank_topology(rank_topology&&) noexcept = default;
    rank_topology& operator=(const rank_topology&) = default;
    rank_topology& operator=(rank_topology&&) noexcept = default;

  public: // member functions
    /** @brief return whether rank is located on this node */
    bool is_local(int rank) const noexcept { return m_rank_set.find(rank) != m_rank_set.end(); }

    /** @brief return number of ranks on this node */
    size_type local_size() const noexcept { return m_rank_set.size(); }

    /** @brief return ranks on this node */
    const set_type& local_ranks() const noexcept { return m_rank_set; }

    /** @brief return local rank number */
    int local_rank() const noexcept { return m_rank; }

    /** @brief return raw mpi communicator */
    auto mpi_comm() const noexcept { return m_comm; }
};

} //namespace oomph
