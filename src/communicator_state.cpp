/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <hwmalloc/numa.hpp>
#include <oomph/config.hpp>
#include <oomph/communicator.hpp>

// paths relative to backend
#include <communicator.hpp>
#include <../communicator_set.hpp>

namespace oomph
{
namespace detail
{
communicator_state::communicator_state(impl_type* impl_,
    std::atomic<std::size_t>* shared_scheduled_recvs)
    //, util::tag_range tr, util::tag_range rtr)
: m_impl{impl_}
, m_shared_scheduled_recvs{shared_scheduled_recvs}
//, m_tag_range(tr)
//, m_reserved_tag_range(rtr)
{
    communicator_set::get().insert(m_impl->m_context, m_impl);
}

communicator_state::~communicator_state()
{
    communicator_set::get().erase(m_impl->m_context, m_impl);
    m_impl->release();
}
} // namespace detail
} // namespace oomph
