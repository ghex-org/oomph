/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <oomph/config.hpp>

// paths relative to backend
#if OOMPH_ENABLE_BARRIER
#include <../communicator_set_impl.hpp>
#else
#include <../communicator_set_noop.hpp>
#endif
#include <../message_buffer.hpp>
#include <../util/heap_pimpl_src.hpp>

OOMPH_INSTANTIATE_HEAP_PIMPL(oomph::communicator_set::impl)

namespace oomph
{

communicator_set&
communicator_set::get()
{
    static communicator_set s;
    return s;
}

communicator_set::communicator_set()
: m_impl{util::make_heap_pimpl<impl>()}
{
}

void
communicator_set::insert(context_impl const* ctxt, communicator_impl* comm)
{
    m_impl->insert(ctxt, comm);
}

void
communicator_set::erase(context_impl const* ctxt, communicator_impl* comm)
{
    m_impl->erase(ctxt, comm);
}

void
communicator_set::erase(context_impl const* ctxt)
{
    m_impl->erase(ctxt);
}

void
communicator_set::progress(context_impl const* ctxt)
{
    m_impl->progress(ctxt);
}

} // namespace oomph
