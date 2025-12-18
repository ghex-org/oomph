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

#include <algorithm>
#include <vector>
#include <boost/lockfree/queue.hpp>

// paths relative to backend
#include <request_state.hpp>

namespace oomph
{

class request_queue
{
  private:
    using element_type = detail::request_state;
    using queue_type = std::vector<element_type*>;

  private: // members
    queue_type               m_queue;
    bool                     in_progress = false;

  public: // ctors
    request_queue()
    {
        m_queue.reserve(256);
    }

  public: // member functions
    std::size_t size() const noexcept { return m_queue.size(); }

    void enqueue(element_type* e)
    {
        e->m_index = m_queue.size();
        m_queue.push_back(e);
    }

    int progress()
    {
        std::cerr << "nccl request_queue::progress\n";
        if (in_progress) return 0;
        in_progress = true;

        const auto qs = size();
        if (qs == 0)
        {
            in_progress = false;
            return 0;
        }

        auto erase_begin = std::remove_if(
            m_queue.begin(), m_queue.end(),
            [](auto& req) {
                std::cerr << "checking if request ready with event " << req->m_req.m_event << "\n";
                if (req->m_req.is_ready()) {
                    auto ptr = req->release_self_ref();
                    std::cerr << "invoking callback on req: " << req << "\n";
                    req->invoke_cb();
                    return true;
                } else {
                    return false;
                }
            }
        );
        auto completed = std::distance(erase_begin, m_queue.end());
        if (completed != 0) {
            std::cerr << "completed " << completed << " requests\n";
        }
        m_queue.erase(erase_begin, m_queue.end());

        in_progress = false;
        return completed;
    }

    bool cancel(element_type*)
    {
        // No cancellation with NCCL.
        return false;
    }
};

class shared_request_queue
{
  private:
    using element_type = detail::shared_request_state;
    using queue_type = boost::lockfree::queue<element_type*, boost::lockfree::fixed_sized<false>,
        boost::lockfree::allocator<std::allocator<void>>>;

  private: // members
    queue_type               m_queue;
    std::atomic<std::size_t> m_size;

  public: // ctors
    shared_request_queue()
    : m_queue(256)
    , m_size(0)
    {
    }

  public: // member functions
    std::size_t size() const noexcept { return m_size.load(); }

    void enqueue(element_type* e)
    {
        m_queue.push(e);
        ++m_size;
    }

    int progress()
    {
        std::cerr << "nccl shared_request_queue::progress\n";

        static thread_local bool                       in_progress = false;
        static thread_local std::vector<element_type*> m_local_queue;
        int                                            found = 0;

        if (in_progress) return 0;
        in_progress = true;

        element_type* e;
        while (m_queue.pop(e))
        {
            if (e->m_req.is_ready())
            {
                std::cerr << "found ready request in shared queue\n";
                found = 1;
                break;
            }
            else
            {
                m_local_queue.push_back(e);
            }
        }

        for (auto x : m_local_queue) m_queue.push(x);
        m_local_queue.clear();

        if (found)
        {
            auto ptr = e->release_self_ref();
            e->invoke_cb();
            --m_size;
        }

        in_progress = false;
        return found;
    }

    bool cancel(element_type*)
    {
        // No cancellation with NCCL.
        return false;
    }
};

} // namespace oomph
