/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <mutex>
#include <set>
#include <memory>

namespace oomph
{
template<typename T, typename Deleter = std::default_delete<T>>
class unique_ptr_set
{
  public:
    using pointer = T*;

  private:
    std::set<pointer>           m_ptrs;
    std::unique_ptr<std::mutex> m_mutex;
    Deleter                     m_deleter;

  public:
    unique_ptr_set(Deleter d = Deleter{})
    : m_mutex{std::make_unique<std::mutex>()}
    , m_deleter{d}
    {
    }

    unique_ptr_set(unique_ptr_set&&) = default;
    unique_ptr_set& operator=(unique_ptr_set&&) = default;

    ~unique_ptr_set()
    {
        if (m_mutex)
            for (auto p : m_ptrs) destroy(p);
    }

  public:
    void insert(pointer p)
    {
        m_mutex->lock();
        m_ptrs.insert(p);
        m_mutex->unlock();
    }

    void remove(pointer p)
    {
        m_mutex->lock();
        m_ptrs.erase(m_ptrs.find(p));
        destroy(p);
        m_mutex->unlock();
    }

  private:
    void destroy(pointer p) { m_deleter(p); }
};

} // namespace oomph
