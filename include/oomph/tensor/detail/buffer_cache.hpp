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

#include <oomph/communicator.hpp>
#include <memory>
#include <vector>
#include <map>
#include <set>

namespace oomph
{
namespace tensor
{
namespace detail
{
template<typename T, typename Id>
struct buffer_cache
{
    std::vector<std::shared_ptr<message_buffer<T>>> m_messages;
    std::map<Id, std::set<std::size_t>>             m_available_idx;

    buffer_cache() noexcept = default;
    buffer_cache(buffer_cache&&) noexcept = default;
    buffer_cache& operator=(buffer_cache&&) noexcept = default;

    std::shared_ptr<message_buffer<T>> operator()(communicator& comm, std::size_t size, Id id)
    {
        std::set<std::size_t>* index_set = nullptr;
        auto                   it = m_available_idx.find(id);
        if (it == m_available_idx.end())
        {
            index_set = &(m_available_idx[id]);
            for (std::size_t i = 0; i < m_messages.size(); ++i) index_set->insert(i);
        }
        else
        {
            index_set = &(it->second);
        }

        auto m_it = std::find_if(index_set->begin(), index_set->end(),
            [this, size](std::size_t i) { return (m_messages[i]->size() == size); });

        if (m_it == index_set->end())
        {
            m_messages.push_back(std::make_shared<message_buffer<T>>(comm.make_buffer<T>(size)));
            for (auto& s : m_available_idx) s.second.insert(m_messages.size() - 1);
            index_set->erase(m_messages.size() - 1);
            return m_messages.back();
        }
        else
        {
            auto res = *m_it;
            index_set->erase(m_it);
            return m_messages[res];
        }
    }

    template<typename Id2>
    std::shared_ptr<message_buffer<T>> operator()(communicator& comm, std::size_t size, Id id,
        buffer_cache<T, Id2>& c2, Id2 id2)
    {
        std::set<std::size_t>* index_set = nullptr;
        auto                   it = m_available_idx.find(id);
        if (it == m_available_idx.end())
        {
            index_set = &(m_available_idx[id]);
            for (std::size_t i = 0; i < m_messages.size(); ++i) index_set->insert(i);
        }
        else
        {
            index_set = &(it->second);
        }

        auto m_it = std::find_if(index_set->begin(), index_set->end(),
            [this, size](std::size_t i) { return (m_messages[i]->size() == size); });

        if (m_it == index_set->end())
        {
            auto res = c2(comm, size, id2);
            m_messages.push_back(res);
            for (auto& s : m_available_idx) s.second.insert(m_messages.size() - 1);
            index_set->erase(m_messages.size() - 1);
            return res;
        }
        else
        {
            auto res = *m_it;
            index_set->erase(m_it);
            return m_messages[res];
        }
    }
};

} // namespace detail
} // namespace tensor
} // namespace oomph
