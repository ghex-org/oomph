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

#include <oomph/util/heap_pimpl.hpp>
#include <oomph/communicator.hpp>

namespace oomph
{
class recv_channel_impl;

void release_recv_channel_buffer(recv_channel_impl*, std::size_t);

class recv_channel_base
{
  protected:
    util::heap_pimpl<recv_channel_impl> m_impl;

    recv_channel_base(communicator& comm, std::size_t size, std::size_t T_size,
        communicator::rank_type src, communicator::tag_type tag, std::size_t levels);

    ~recv_channel_base();

    void* get(std::size_t& index);

    recv_channel_impl* get_impl() noexcept;

  public:
    void connect();

    std::size_t capacity();
};

template<typename T>
class recv_channel : public recv_channel_base
{
    using base = recv_channel_base;

  public:
    class buffer
    {
        // message_buffer<T> m_buffer;
        friend class recv_channel<T>;

      private:
        T*                 m_ptr = nullptr;
        std::size_t        m_size;
        std::size_t        m_index;
        recv_channel_impl* m_recv_channel_impl;

      public:
        buffer() = default;

        buffer(buffer&& other)
        : m_ptr{std::exchange(other.m_ptr, nullptr)}
        , m_size{other.m_size}
        , m_index{other.m_index}
        , m_recv_channel_impl{other.m_recv_channel_impl}
        {
        }

        buffer& operator=(buffer&& other)
        {
            release();
            m_ptr = std::exchange(other.m_ptr, nullptr);
            m_size = other.m_size;
            m_index = other.m_index;
            m_recv_channel_impl = other.m_recv_channel_impl;
            return *this;
        }

        ~buffer() { release(); }

      private:
        buffer(T* ptr, std::size_t size_, std::size_t index, recv_channel_impl* rc)
        : m_ptr{ptr}
        , m_size{size_}
        , m_index{index}
        , m_recv_channel_impl{rc}
        {
        }

      public:
        operator bool() const noexcept { return m_ptr; }

        std::size_t size() const noexcept { return m_size; }

        T*       data() noexcept { return m_ptr; }
        T const* data() const noexcept { return m_ptr; }
        T*       begin() noexcept { return data(); }
        T const* begin() const noexcept { return data(); }
        T*       end() noexcept { return data() + size(); }
        T const* end() const noexcept { return data() + size(); }
        T const* cbegin() const noexcept { return data(); }
        T const* cend() const noexcept { return data() + size(); }

        void release()
        {
            if (m_ptr)
            {
                release_recv_channel_buffer(m_recv_channel_impl, m_index);
                m_ptr = nullptr;
            }
        }
    };

    //class request
    //{
    //    class impl;
    //    bool is_ready_local();
    //    bool is_ready_remote();
    //    void wait_local();
    //    void wait_remote();
    //};

  private:
    std::size_t m_size;

  public:
    recv_channel(communicator& comm, std::size_t size, communicator::rank_type src,
        communicator::tag_type tag, std::size_t levels)
    : base(comm, size, sizeof(T), src, tag, levels)
    , m_size{size}
    {
    }
    recv_channel(recv_channel const&) = delete;
    recv_channel(recv_channel&&) = default;

    //void connect();

    //std::size_t capacity() const noexcept;

    //buffer make_buffer();

    buffer get()
    {
        T*          ptr = nullptr;
        std::size_t index;
        do
        {
            ptr = (T*)base::get(index);
        }
        //while (!ptr);
        while (false);
        return {ptr, m_size, index, base::get_impl()};
    }

    //request put(buffer& b);
};

} // namespace oomph
