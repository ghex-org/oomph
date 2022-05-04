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

//#include <oomph/communicator.hpp>
//#include "./request.hpp"
#include "./callback_queue.hpp"
//#include <vector>
//#include <algorithm>
#include <atomic>
#include <boost/lockfree/queue.hpp>

namespace oomph
{

class shared_callback_queue
{
  public: // member types
    using cb_type = util::unique_function<void()>;
    using cb_interface_type = typename cb_type::interface_t;
    //using handle_type = detail::request_state;
    //using handle_ptr = communicator::shared_request_ptr;

    struct element_type
    {
        mpi_request        m_request;
        cb_interface_type* m_cb_ptr = nullptr;
        //handle_ptr  m_handle;

        element_type(mpi_request req, cb_type&& cb) //, handle_ptr&& h)
        : m_request{req}
        , m_cb_ptr{cb.release()}
        //, m_handle{std::move(h)}
        {
        }

        element_type() noexcept = default;
        element_type(element_type const&) noexcept = default;
        element_type(element_type&&) noexcept = default;
        element_type& operator=(element_type const&) noexcept = default;
        element_type& operator=(element_type&&) noexcept = default;
    };

    using queue_type = boost::lockfree::queue<element_type, boost::lockfree::fixed_sized<false>,
        boost::lockfree::allocator<std::allocator<void>>>;

  private: // members
    queue_type               m_queue;
    std::atomic<std::size_t> m_size;
    //bool       in_progress = false;

  public: // ctors
    shared_callback_queue()
    : m_queue(256)
    , m_size(0)
    {
    }

  public: // member functions
    std::size_t size() const noexcept { return m_size.load(); }

    void enqueue(mpi_request const& req, cb_type&& cb) //, handle_ptr&& h)
    {
        element_type e(req, std::move(cb)); //, std::move(h));
        //m_queue.push_back(element_type{req, std::move(cb), std::move(h)});
        //m_queue.back().m_handle->reserved()->m_index = m_queue.size() - 1;
        m_queue.push(e);
        ++m_size;
    }

    int progress()
    {
        static thread_local bool                      in_progress = false;
        static thread_local std::vector<element_type> m_local_queue;
        int                                           found = 0;

        if (in_progress) return 0;
        in_progress = true;

        element_type e;
        while (m_queue.pop(e))
        {
            if (e.m_request.is_ready())
            {
                found = 1;
                break;
            }
            else
            {
                m_local_queue.push_back(e);
            }
        }

        for (auto const& x : m_local_queue) m_queue.push(x);
        m_local_queue.clear();

        if (found)
        {
            //e.m_cb_ptr->invoke();
            //delete e.m_cb_ptr;
            cb_type cb{e.m_cb_ptr};
            cb();
            --m_size;
        }

        in_progress = false;
        return found;
    }
};

} // namespace oomph
