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

#include <oomph/communicator.hpp>
#include "./request.hpp"
#include <vector>
#include <algorithm>

namespace oomph
{

struct detail::request_state::reserved_t
{
    std::size_t m_index;
};

class callback_queue
{
  public: // member types
    using cb_type = util::unique_function<void()>;
    using handle_type = detail::request_state;
    using handle_ptr = communicator::shared_request_ptr;

    struct element_type
    {
        mpi_request m_request;
        cb_type     m_cb;
        handle_ptr  m_handle;

        element_type(mpi_request req, cb_type&& cb, handle_ptr&& h)
        : m_request{req}
        , m_cb{std::move(cb)}
        , m_handle{std::move(h)}
        {
        }
        element_type(element_type const&) = delete;
        element_type(element_type&&) = default;
        element_type& operator=(element_type const&) = delete;
        element_type& operator=(element_type&& other) = default;
    };

    using queue_type = std::vector<element_type>;

  private: // members
    queue_type               m_queue;
    queue_type               m_ready_queue;
    bool                     in_progress = false;
    std::vector<MPI_Request> reqs;
    std::vector<int>         indices;

  public: // ctors
    callback_queue()
    {
        m_queue.reserve(256);
        m_ready_queue.reserve(256);
    }

  public: // member functions
    void enqueue(mpi_request const& req, cb_type&& cb, handle_ptr&& h)
    {
        m_queue.push_back(element_type{req, std::move(cb), std::move(h)});
        m_queue.back().m_handle->reserve()->m_index = m_queue.size() - 1;
    }

    auto size() const noexcept { return m_queue.size(); }

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
        reqs.resize(0);
        reqs.reserve(qs);
        indices.resize(qs + 1);

        std::transform(m_queue.begin(), m_queue.end(), std::back_inserter(reqs),
            [](auto& e) { return e.m_request.m_req; });

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
            auto& e = m_queue[i];
            if ((int)i == indices[k])
            {
                m_ready_queue.push_back(std::move(e));
                ++k;
            }
            else if (i > j)
            {
                e.m_handle->reserve()->m_index = j;
                m_queue[j] = std::move(e);
                ++j;
            }
            else
            {
                ++j;
            }
        }
        reqs.clear();
        m_queue.erase(m_queue.end() - m_ready_queue.size(), m_queue.end());

        int completed = m_ready_queue.size();
        for (auto& e : m_ready_queue) e.m_cb();

        in_progress = false;
        return completed;
    }

    bool cancel(std::size_t index)
    {
        if (m_queue[index].m_request.cancel())
        {
            if (index + 1 < m_queue.size())
            {
                m_queue[index] = std::move(m_queue.back());
                m_queue[index].m_handle->reserve()->m_index = index;
            }
            m_queue.pop_back();
            return true;
        }
        else
            return false;
    }
};

} // namespace oomph
