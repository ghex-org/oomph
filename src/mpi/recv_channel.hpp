/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/recv_channel.hpp>
#include "./channel_base.hpp"
#include <hwmalloc/numa.hpp>

namespace oomph
{
class recv_channel_impl : public channel_base
{
    using base = channel_base;
    using flag_basic_type = typename base::flag_basic_type;
    using flag_type = typename base::flag_type;
    using pointer = typename base::pointer;
    using handle_type = typename base::handle_type;
    using key_type = typename base::key_type;

  private:
    communicator::impl* m_comm;
    pointer             m_buffer;
    key_type            m_local_key;

  public:
    recv_channel_impl(communicator::impl* impl_, std::size_t size, std::size_t T_size,
        communicator::rank_type src, communicator::tag_type tag, std::size_t levels)
    : base(size, T_size, src, tag, levels)
    , m_comm(impl_)
    , m_buffer{m_comm->get_heap().allocate(
          levels * base::buffer_size() * T_size, hwmalloc::numa().local_node())}
    , m_local_key{m_buffer.handle().get_remote_key()}
    {
        m_comm->m_context->lock(src);
        OOMPH_CHECK_MPI_RESULT(MPI_Isend(&m_local_key, sizeof(key_type), MPI_BYTE,
            base::m_remote_rank, base::m_tag, m_comm->get_comm(), &(base::m_init_req)));
    }
    recv_channel_impl(recv_channel_impl const&) = delete;
    recv_channel_impl(recv_channel_impl&&) = delete;

    ~recv_channel_impl()
    {
    }
    
    //void connect() {}
 
    std::size_t capacity()
    {
        return base::m_capacity;
    }

    void* get(std::size_t& index)
    {
        index = 0;
        return nullptr;
    }

    void release(std::size_t index)
    {
    }
};

void release_recv_channel_buffer(recv_channel_impl* rc, std::size_t index)
{
    rc->release(index);
}

} // namespace oomph
