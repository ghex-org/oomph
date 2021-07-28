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

#include "./context_base.hpp"

namespace oomph
{
template<typename Communicator>
class communicator_base
{
  public:
    using rank_type = communicator::rank_type;

  protected:
    context_base* m_context;

    communicator_base(context_base* ctxt)
    : m_context(ctxt)
    {
    }

  public:
    rank_type            rank() const noexcept { return m_context->rank(); }
    rank_type            size() const noexcept { return m_context->size(); }
    MPI_Comm             mpi_comm() const noexcept { return m_context->get_comm(); }
    rank_topology const& topology() const noexcept { return m_context->topology(); }
    void release() { m_context->deregister_communicator(static_cast<Communicator*>(this)); }
    bool is_local(rank_type rank) const noexcept { return topology().is_local(rank); }
};
} // namespace oomph
