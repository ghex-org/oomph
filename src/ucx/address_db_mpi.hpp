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

#include <map>
#include <vector>

#include <oomph/util/mpi_error.hpp>

// paths relative to backend
#include <error.hpp>
#include <endpoint.hpp>
#include <address.hpp>

namespace oomph
{
struct address_db_mpi
{
    using key_t = rank_type;
    using value_t = address_t;

    MPI_Comm    m_mpi_comm;
    const key_t m_rank;
    const key_t m_size;

    value_t                  m_value;
    std::map<key_t, value_t> m_address_map;

    address_db_mpi(MPI_Comm comm)
    : m_mpi_comm{comm}
    , m_rank{[](MPI_Comm c) {
        int r;
        OOMPH_CHECK_MPI_RESULT(MPI_Comm_rank(c, &r));
        return r;
    }(comm)}
    , m_size{[](MPI_Comm c) {
        int s;
        OOMPH_CHECK_MPI_RESULT(MPI_Comm_size(c, &s));
        return s;
    }(comm)}
    {
    }

    address_db_mpi(const address_db_mpi&) = delete;
    address_db_mpi(address_db_mpi&&) = default;

    key_t rank() const noexcept { return m_rank; }
    key_t size() const noexcept { return m_size; }
    int   est_size() const noexcept { return m_size; }

    value_t find(key_t k)
    {
        auto it = m_address_map.find(k);
        if (it != m_address_map.end()) { return it->second; }
        throw std::runtime_error("Cound not find peer address in the MPI address xdatabase.");
    }

    void init(const value_t& addr)
    {
        m_value = addr;
        m_address_map[m_rank] = addr;
        for (key_t r = 0; r < m_size; ++r)
        {
            if (r == m_rank)
            {
                std::size_t size = m_value.size();
                OOMPH_CHECK_MPI_RESULT(
                    MPI_Bcast(&size, sizeof(std::size_t), MPI_BYTE, r, m_mpi_comm));
                OOMPH_CHECK_MPI_RESULT(
                    MPI_Bcast(m_value.data(), m_value.size(), MPI_BYTE, r, m_mpi_comm));
            }
            else
            {
                std::size_t size;
                OOMPH_CHECK_MPI_RESULT(
                    MPI_Bcast(&size, sizeof(std::size_t), MPI_BYTE, r, m_mpi_comm));
                value_t addr(size);
                OOMPH_CHECK_MPI_RESULT(MPI_Bcast(addr.data(), size, MPI_BYTE, r, m_mpi_comm));
                m_address_map[r] = addr;
            }
        }
    }
};

} // namespace oomph
