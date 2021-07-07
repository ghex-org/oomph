#pragma once

#include <oomph/util/heap_pimpl.hpp>
#include <oomph/communicator.hpp>

namespace oomph
{
class recv_channel_impl;

class recv_channel_base
{
  protected:
    util::heap_pimpl<recv_channel_impl> m_impl;

    recv_channel_base(communicator& comm, std::size_t size, std::size_t T_size,
        communicator::rank_type src, communicator::tag_type tag, std::size_t levels);

    ~recv_channel_base();

  public:
    void connect();
};

template<typename T>
class recv_channel : public recv_channel_base
{
    using base = recv_channel_base;

  public:
    //class buffer
    //{
    //    message_buffer<T> m_buffer;
    //};

    //class request
    //{
    //    class impl;
    //    bool is_ready_local();
    //    bool is_ready_remote();
    //    void wait_local();
    //    void wait_remote();
    //};

  public:
    recv_channel(communicator& comm, std::size_t size, communicator::rank_type src,
        communicator::tag_type tag, std::size_t levels)
    : base(comm, size, sizeof(T), src, tag, levels)
    {
    }
    recv_channel(recv_channel const&) = delete;
    recv_channel(recv_channel&&) = default;

    //void connect();

    //std::size_t capacity() const noexcept;

    //buffer make_buffer();

    //request put(buffer& b);
};

} // namespace oomph
