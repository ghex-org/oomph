/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <mutex>
#include <set>
#include <map>

// paths relative to backend
#include <context.hpp>
#include <communicator.hpp>
#include <../communicator_set.hpp>
#include <../thread_id.hpp>

namespace oomph
{

struct communicator_set::impl
{
    using set_type = std::set<communicator_impl*>;
    using map_type = std::map<std::uintptr_t, set_type>;
    using mutex = std::mutex;
    using lock_guard = std::lock_guard<mutex>;

    mutex                                   m_mtx;
    std::map<context_impl const*, map_type> m_map;

    void insert(context_impl const* ctxt, communicator_impl* comm)
    {
        auto const& _tid = tid();
        lock_guard  lock(m_mtx);
        m_map[ctxt][_tid].insert(comm);
    }

    void erase(context_impl const* ctxt, communicator_impl* comm)
    {
        auto const& _tid = tid();
        lock_guard  lock(m_mtx);
        m_map[ctxt][_tid].erase(comm);
    }

    void erase(context_impl const* ctxt)
    {
        lock_guard lock(m_mtx);
        m_map.erase(ctxt);
    }

    void progress(context_impl const* ctxt)
    {
        auto const& _tid = tid();
        lock_guard  lock(m_mtx);
        auto&       s = m_map[ctxt][_tid];
        for (auto c : s) c->progress();
    }
};

} // namespace oomph
