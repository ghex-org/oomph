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

#include <variant>
//
#include <oomph/request.hpp>
//
#include <libfatbat/logging.hpp>
#include <libfatbat/operation_context_base.hpp>
//
namespace oomph::libfabric
{

MAKE_LOGGER(opctx_deb, "opctxt")

// This struct holds the ready state of a future
// we must also store the context used in libfabric, in case
// a request is cancelled - fi_cancel(...) needs it
struct operation_context : public libfatbat::operation_context_base<operation_context>
{
    std::variant<oomph::detail::request_state*, oomph::detail::shared_request_state*> m_req;

    template<typename RequestState>
    operation_context(RequestState* req)
    : operation_context_base()
    , m_req{req}
    {
        LIBFATBAT_SCOPE(opctx_deb, "{} {} request {}", static_cast<void*>(this), __func__,
            static_cast<void*>(req));
    }

    // --------------------------------------------------------------------
    // When a completion returns FI_ECANCELED, this is called
    void handle_cancelled();

    // --------------------------------------------------------------------
    // Called when a tagged recv completes
    int handle_tagged_recv_completion_impl(void* user_data);

    // --------------------------------------------------------------------
    // Called when a tagged send completes
    int handle_tagged_send_completion_impl(void* user_data);
};

} // namespace oomph::libfabric
