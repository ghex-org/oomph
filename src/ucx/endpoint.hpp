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

#include <oomph/communicator.hpp>
#include <oomph/util/moved_bit.hpp>

// paths relative to backend
#include <address.hpp>

namespace oomph
{
struct endpoint_t
{
    rank_type       m_rank;
    ucp_ep_h        m_ep;
    ucp_worker_h    m_worker;
    util::moved_bit m_moved;

    endpoint_t() noexcept
    : m_moved(true)
    {
    }
    endpoint_t(rank_type rank, ucp_worker_h local_worker, const address_t& remote_worker_address)
    : m_rank(rank)
    , m_worker{local_worker}
    {
        ucp_ep_params_t ep_params;
        ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
        ep_params.address = remote_worker_address.get();
        OOMPH_CHECK_UCX_RESULT(ucp_ep_create(local_worker, &ep_params, &(m_ep)));
    }

    endpoint_t(const endpoint_t&) = delete;
    endpoint_t& operator=(const endpoint_t&) = delete;
    endpoint_t(endpoint_t&& other) noexcept = default;
    endpoint_t& operator=(endpoint_t&& other) = delete;

    struct close_handle
    {
        bool             m_done;
        ucp_worker_h     m_ucp_worker;
        ucs_status_ptr_t m_status;

        close_handle()
        : m_done{true}
        {
        }

        close_handle(ucp_worker_h worker, ucs_status_ptr_t status)
        : m_done{false}
        , m_ucp_worker{worker}
        , m_status{status}
        {
        }

        close_handle(close_handle&& other)
        : m_done{std::exchange(other.m_done, true)}
        , m_ucp_worker{other.m_ucp_worker}
        , m_status{other.m_status}
        {
        }

        bool ready()
        {
            progress();
            return m_done;
        }

        void progress()
        {
            if (!m_done)
            {
                ucp_worker_progress(m_ucp_worker);
                if (UCS_OK != ucp_request_check_status(m_status))
                {
                    ucp_request_free(m_status);
                    m_done = true;
                }
            }
        }
    };

    close_handle close()
    {
        if (m_moved) return {};
        ucs_status_ptr_t ret = ucp_ep_close_nb(m_ep, UCP_EP_CLOSE_MODE_FLUSH);
        if (UCS_OK == reinterpret_cast<std::uintptr_t>(ret)) return {};
        if (UCS_PTR_IS_ERR(ret)) return {};
        return {m_worker, ret};
    }

    //operator bool() const noexcept { return m_moved; }
    operator ucp_ep_h() const noexcept { return m_ep; }

    rank_type       rank() const noexcept { return m_rank; }
    ucp_ep_h&       get() noexcept { return m_ep; }
    const ucp_ep_h& get() const noexcept { return m_ep; }
};

} // namespace oomph
