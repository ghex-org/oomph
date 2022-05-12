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

#include "./request.hpp"
#include <deque>
#include <mutex>

namespace oomph
{

class shared_request_queue
{
  private:
    using element_type = std::shared_ptr<detail::shared_request_state>;
    using queue_type = std::deque<element_type>;
    using mutex_type = std::mutex;
    using lock_type = std::lock_guard<mutex_type>;

    queue_type m_queue;
    mutex_type m_mtx;

  public:
    shared_request_queue() = default;

    void push(element_type r)
    {
        lock_type lck(m_mtx);
        m_queue.push_back(std::move(r));
    }

    std::size_t progress()
    {
        std::size_t size = 0ul;
        std::size_t progressed = 0ul;
        {
            lock_type lck(m_mtx);
            size = m_queue.size();
        }
        if (size==0) return progressed;
        for (std::size_t i=0; i<size; ++i)
        {
            m_mtx.lock();
            if (m_queue.empty())
            {
                m_mtx.unlock();
                break;
            }
            element_type s = std::move(m_queue.front());
            m_queue.pop_front();
            m_mtx.unlock();
            if (s->m_req.is_ready())
            {
                s->invoke_cb();
                ++progressed;
            }
            else
            {
                lock_type lck(m_mtx);
                m_queue.push_back(std::move(s));
            }
        }
        return progressed;
    }
};

} // namespace oomph
