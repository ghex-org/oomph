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

#include <oomph/util/mpi_error.hpp>
#include "./context.hpp"

namespace oomph
{
class channel_base
{
  protected:
    using heap_type = context_impl::heap_type;
    using pointer = heap_type::pointer;
    using handle_type = typename pointer::handle_type;
    using key_type = typename handle_type::key_type;
    using flag_basic_type = key_type;
    using flag_type = flag_basic_type volatile;

  protected:
    //heap_type&              m_heap;
    std::size_t             m_size;
    std::size_t             m_T_size;
    std::size_t             m_levels;
    std::size_t             m_capacity;
    communicator::rank_type m_remote_rank;
    communicator::tag_type  m_tag;
    bool                    m_connected = false;
    MPI_Request             m_init_req;

  public:
    channel_base(/*heap_type& h,*/ std::size_t size, std::size_t T_size,
        communicator::rank_type remote_rank, communicator::tag_type tag, std::size_t levels)
    //: m_heap{h}
    : m_size{size}
    , m_T_size{T_size}
    , m_levels{levels}
    , m_capacity{levels}
    , m_remote_rank{remote_rank}
    , m_tag{tag}
    {
    }

    void connect()
    {
        OOMPH_CHECK_MPI_RESULT(MPI_Wait(&m_init_req, MPI_STATUS_IGNORE));
        m_connected = true;
    }

  protected:
    // index of flag in buffer (in units of flag_basic_type)
    std::size_t flag_offset() const noexcept
    {
        return (m_size * m_T_size + 2 * sizeof(flag_basic_type) - 1) / sizeof(flag_basic_type) - 1;
    }
    // number of elements of type T (including padding)
    std::size_t buffer_size() const noexcept
    {
        return ((flag_offset() + 1) * sizeof(flag_basic_type) + m_T_size - 1) / m_T_size;
    }
    // pointer to flag location for a given buffer
    void* flag_ptr(void* ptr) const noexcept
    {
        return (void*)((char*)ptr + flag_offset() * sizeof(flag_basic_type));
    }
};

} // namespace oomph
