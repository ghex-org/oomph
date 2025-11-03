/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hwmalloc/numa.hpp>
#include <oomph/send_channel.hpp>

// paths relative to backend
#include <channel_base.hpp>

namespace oomph
{
class send_channel_impl : public channel_base
{
    using base = channel_base;
    using flag_basic_type = typename base::flag_basic_type;
    using flag_type = typename base::flag_type;
    using pointer = typename base::pointer;
    using handle_type = typename base::handle_type;
    using key_type = typename base::key_type;

    communicator::impl* m_comm;
    key_type            m_remote_key;

  public:
    send_channel_impl(communicator::impl* impl_, std::size_t size, std::size_t T_size,
        communicator::rank_type dst, communicator::tag_type tag, std::size_t levels)
    : base(size, T_size, dst, tag, levels)
    , m_comm(impl_)
    {
        m_comm->m_context->lock(dst);
        OOMPH_CHECK_MPI_RESULT(MPI_Irecv(&m_remote_key, sizeof(key_type), MPI_BYTE,
            base::m_remote_rank, base::m_tag, m_comm->get_comm(), &(base::m_init_req)));
    }
    send_channel_impl(send_channel_impl const&) = delete;
    send_channel_impl(send_channel_impl&&) = delete;

};

} // namespace oomph
