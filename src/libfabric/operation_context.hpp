/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <thread>

#include <rdma/fabric.h>
//
#include <oomph/util/unique_function.hpp>
#include <oomph/request.hpp>
//
#include "operation_context_base.hpp"
//
#include <boost/lockfree/queue.hpp>
//
namespace oomph
{
namespace libfabric
{

// cppcheck-suppress ConfigurationNotChecked
static NS_DEBUG::enable_print<false> ctx_deb("CONTEXT");

struct operation_context;

struct queue_data
{
    using cb_ptr_t = util::detail::unique_function<void>*;
    //
    operation_context* ctxt;
    cb_ptr_t           user_cb_;
};

// This struct holds the ready state of a future
// we must also store the context used in libfabric, in case
// a request is cancelled - fi_cancel(...) needs it
struct operation_context : public operation_context_base<operation_context>
{
  public:
    using cb_ptr_t = util::detail::unique_function<void>*;
    using lockfree_queue = boost::lockfree::queue<queue_data, boost::lockfree::fixed_sized<false>,
        boost::lockfree::allocator<std::allocator<void>>>;
    //
    cb_ptr_t               user_cb_;
    lockfree_queue * const callback_queue_;
    lockfree_queue * const cancel_queue_;
    std::thread::id  const thread_id_;

    operation_context(cb_ptr_t user_cb, lockfree_queue* queue, lockfree_queue* cancelq)
    : operation_context_base(ctx_any)
    , user_cb_(user_cb)
    , callback_queue_(queue)
    , cancel_queue_(cancelq)
    , thread_id_(std::this_thread::get_id())
    {
        [[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__, user_cb_);
    }

    ~operation_context() {
        assert(std::this_thread::get_id() == thread_id_);
        delete user_cb_;
    }

    // --------------------------------------------------------------------
    // When a completion returns FI_ECANCELED, this is called
    void handle_cancelled()
    {
        [[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__, user_cb_);
        // enqueue the cancelled/callback
        while (!cancel_queue_->push(queue_data{this, user_cb_})) {}
    }

    // --------------------------------------------------------------------
    // Called when a recv completes
    int handle_tagged_recv_completion_impl()
    {
        [[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__, user_cb_);
        if (std::this_thread::get_id() == thread_id_)
        {
            user_cb_->invoke();
            delete user_cb_;
        }
        else
        {
            // enqueue the callback
            while (!callback_queue_->push(queue_data{this, user_cb_})) {}
        }
        return 1;
    }

    // --------------------------------------------------------------------
    // Called when a send completes
    int handle_tagged_send_completion_impl()
    {
        [[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__, user_cb_);
        if (std::this_thread::get_id() == thread_id_)
        {
            user_cb_->invoke();
            delete user_cb_;
        }
        else
        {
            // enqueue the callback
            while (!callback_queue_->push(queue_data{this, user_cb_})) {}
        }
        return 1;
    }
};

} // namespace libfabric
} // namespace oomph
