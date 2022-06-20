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

//#include <thread>
#include <variant>

//#include <rdma/fabric.h>
//
//#include <oomph/util/unique_function.hpp>
#include <oomph/request.hpp>
//
#include "operation_context_base.hpp"
//
//#include <boost/lockfree/queue.hpp>
//
namespace oomph
{
namespace libfabric
{

// cppcheck-suppress ConfigurationNotChecked
static NS_DEBUG::enable_print<false> ctx_deb("CONTEXT");

// This struct holds the ready state of a future
// we must also store the context used in libfabric, in case
// a request is cancelled - fi_cancel(...) needs it
struct operation_context : public operation_context_base<operation_context>
{
    std::variant<oomph::detail::request_state*, oomph::detail::shared_request_state*> m_req;

    template<typename RequestState>
    operation_context(RequestState* req)
    : operation_context_base(ctx_any)
    , m_req{req}
    {
        //[[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__, user_cb_);
    }

    //~operation_context()
    //{
    //    assert(std::this_thread::get_id() == thread_id_);
    //    delete user_cb_;
    //}

    // --------------------------------------------------------------------
    // When a completion returns FI_ECANCELED, this is called
    void handle_cancelled();
    //{
    //    [[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__, user_cb_);
    //    // enqueue the cancelled/callback
    //    while (!cancel_queue_->push(queue_data{this, user_cb_})) {}
    //}

    // --------------------------------------------------------------------
    // Called when a recv completes
    int handle_tagged_recv_completion_impl(void* user_data);
    //{
    //    [[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__, user_cb_);
    //    if (std::this_thread::get_id() == thread_id_)
    //    {
    //        user_cb_->invoke();
    //        delete user_cb_;
    //    }
    //    else
    //    {
    //        // enqueue the callback
    //        while (!callback_queue_->push(queue_data{this, user_cb_})) {}
    //    }
    //    return 1;
    //}

    // --------------------------------------------------------------------
    // Called when a send completes
    int handle_tagged_send_completion_impl(void* user_data);
    //{
    //    [[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__, user_cb_);
    //    if (std::this_thread::get_id() == thread_id_)
    //    {
    //        user_cb_->invoke();
    //        delete user_cb_;
    //    }
    //    else
    //    {
    //        // enqueue the callback
    //        while (!callback_queue_->push(queue_data{this, user_cb_})) {}
    //    }
    //    return 1;
    //}
};

} // namespace libfabric
} // namespace oomph
