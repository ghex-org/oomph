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
    queue_type               m_ready_queue;
    bool                     in_progress = false;
    std::vector<MPI_Request> reqs;
    std::vector<int>         indices;

  public: // ctors
    request_queue()
    {
        m_queue.reserve(256);
        m_ready_queue.reserve(256);
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
        if (in_progress) return 0;
        in_progress = true;

        const auto qs = size();
        if (qs == 0)
        {
            in_progress = false;
            return 0;
        }

        m_ready_queue.clear();

        m_ready_queue.reserve(qs);
        //reqs.resize(0);
        reqs.clear();
        reqs.reserve(qs);
        indices.resize(qs + 1);

        std::transform(m_queue.begin(), m_queue.end(), std::back_inserter(reqs),
            [](auto e) { return e->m_req.m_req; });

        int outcount;
        OOMPH_CHECK_MPI_RESULT(
            MPI_Testsome(qs, reqs.data(), &outcount, indices.data(), MPI_STATUSES_IGNORE));

        if (outcount == 0)
        {
            in_progress = false;
            return 0;
        }

        indices[outcount] = qs;

        std::size_t k = 0;
        std::size_t j = 0;
        for (std::size_t i = 0; i < qs; ++i)
        {
            auto e = m_queue[i];
            if ((int)i == indices[k])
            {
                m_ready_queue.push_back(e);
                ++k;
            }
            else if (i > j)
            {
                e->m_index = j;
                m_queue[j] = e;
                ++j;
            }
            else
            {
                ++j;
            }
        }
        m_queue.erase(m_queue.end() - m_ready_queue.size(), m_queue.end());

        int completed = m_ready_queue.size();
        for (auto e : m_ready_queue)
        {
            auto ptr = e->release_self_ref();
            e->invoke_cb();
        }

        in_progress = false;
        return completed;
    }

    bool cancel(element_type* e)
    {
        auto const index = e->m_index;
        if (m_queue[index]->m_req.cancel())
        {
            auto ptr = e->release_self_ref();
            e->set_canceled();
            if (index + 1 < m_queue.size())
            {
                m_queue[index] = m_queue.back();
                m_queue[index]->m_index = index;
            }
            m_queue.pop_back();
            return true;
        }
        else
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

    bool cancel(element_type* e)
    {
        static thread_local std::vector<element_type*> m_local_queue;
        m_local_queue.clear();

        bool canceled = false;
        m_queue.consume_all(
            [q = &m_local_queue, e, &canceled](element_type* x)
            {
                if (e == x)
                {
                    if (e->m_req.cancel())
                    {
                        auto ptr = e->release_self_ref();
                        e->set_canceled();
                        canceled = true;
                    }
                    else
                        q->push_back(x);
                }
                else
                    q->push_back(x);
            });

        for (auto x : m_local_queue) m_queue.push(x);

        return canceled;
    }
};

} // namespace oomph
