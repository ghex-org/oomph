#pragma once

#include <oomph/util/heap_pimpl.hpp>
#include <oomph/communicator.hpp>

namespace oomph
{
class send_channel_impl;

class send_channel_base
{
  protected:
    util::heap_pimpl<send_channel_impl> m_impl;

    send_channel_base(communicator& comm, std::size_t size, std::size_t T_size,
        communicator::rank_type dst, communicator::tag_type tag, std::size_t levels);

    ~send_channel_base();

  public:
    void connect();
};

template<typename T>
class send_channel : public send_channel_base
{
    using base = send_channel_base;

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
    send_channel(communicator& comm, std::size_t size, communicator::rank_type dst,
        communicator::tag_type tag, std::size_t levels)
    : base(comm, size, sizeof(T), dst, tag, levels)
    {
    }
    send_channel(send_channel const&) = delete;
    send_channel(send_channel&&) = default;

    //void connect();

    //std::size_t capacity() const noexcept;

    //buffer make_buffer();

    //request put(buffer& b);
};

} // namespace oomph
