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

#include <oomph/message_buffer.hpp>
#include <oomph/request.hpp>
#include <oomph/util/mpi_error.hpp>
#include <functional>
#include <vector>
#include <atomic>
#include <cassert>

namespace oomph
{
class context;
//class context_impl;
class send_channel_base;
class recv_channel_base;

class communicator
{
  public:
    class impl;
    using rank_type = int;
    using tag_type = int;

  public:
    static constexpr rank_type any_source = -1;
    static constexpr tag_type  any_tag = -1;

  private:
    friend class context;
    friend class send_channel_base;
    friend class recv_channel_base;

  private:
    impl* m_impl;
    //context_impl* m_context_impl;

  private:
    communicator(impl* impl_ /*, context_impl* c_impl_*/);

  public:
    communicator(communicator const&) = delete;
    communicator(communicator&&);
    communicator& operator=(communicator const&) = delete;
    communicator& operator=(communicator&&);
    ~communicator();

  public:
    rank_type rank() const noexcept;
    rank_type size() const noexcept;
    MPI_Comm  mpi_comm() const noexcept;
    bool      is_local(rank_type rank) const noexcept;

    template<typename T>
    message_buffer<T> make_buffer(std::size_t size)
    {
        return {make_buffer_core(size * sizeof(T)), size};
    }

    template<typename T>
    [[nodiscard]] request send(message_buffer<T> const& msg, rank_type dst, tag_type tag)
    {
        assert(msg);
        return send(msg.m, msg.size() * sizeof(T), dst, tag);
    }

    template<typename T>
    [[nodiscard]] request recv(message_buffer<T>& msg, rank_type src, tag_type tag)
    {
        assert(msg);
        return recv(msg.m, msg.size() * sizeof(T), src, tag);
    }

    template<typename T, typename CallBack>
    void send(message_buffer<T>&& msg, rank_type dst, tag_type tag, CallBack&& callback)
    {
        assert(msg);
        const auto s = msg.size();
        send(std::move(msg.m), s * sizeof(T), dst, tag,
            [s, cb = std::forward<CallBack>(callback)](detail::message_buffer m, rank_type dst,
                tag_type tag) mutable { cb(message_buffer<T>(std::move(m), s), dst, tag); });
    }

    template<typename T, typename CallBack>
    void recv(message_buffer<T>&& msg, rank_type src, tag_type tag, CallBack&& callback)
    {
        assert(msg);
        const auto s = msg.size();
        recv(std::move(msg.m), s * sizeof(T), src, tag,
            [s, cb = std::forward<CallBack>(callback)](detail::message_buffer m, rank_type src,
                tag_type tag) mutable { cb(message_buffer<T>(std::move(m), s), src, tag); });
    }

    template<typename T>
    [[nodiscard]] std::vector<request> send_multi(
        message_buffer<T> const& msg, std::vector<rank_type> const& neighs, tag_type tag)
    {
        assert(msg);
        std::vector<request> reqs;
        reqs.reserve(neighs.size());
        for (auto id : neighs) reqs.push_back(send(msg, id, tag));
        return reqs;
    }

    template<typename T, typename CallBack>
    void send_multi(message_buffer<T>&& msg, std::vector<rank_type> const& neighs, tag_type tag,
        CallBack&& callback)
    {
        assert(msg);
        const auto s = msg.size();
        auto       counter = new std::atomic<int>(neighs.size());
        auto       neighs_ptr = new std::vector<rank_type>(neighs);
        const auto n_neighs = neighs.size();
        auto       cb = std::forward<CallBack>(callback);

        auto multi_cb = [s, counter, neighs_ptr, cb](
                            detail::message_buffer m, rank_type, tag_type tag) mutable {
            if ((--(*counter)) == 0)
            {
                cb(message_buffer<T>(std::move(m), s), *neighs_ptr, tag);
                delete counter;
                delete neighs_ptr;
            }
            else
            {
                m.clear();
            }
        };

        std::size_t i = 0;
        for (auto id : neighs)
        {
            if ((i + 1) < n_neighs) send(clone_buffer(msg.m), s * sizeof(T), id, tag, multi_cb);
            else
                send(std::move(msg.m), s * sizeof(T), id, tag, multi_cb);
            ++i;
        }
    }

    void progress();

  private:
    detail::message_buffer make_buffer_core(std::size_t size);

    request send(detail::message_buffer const& msg, std::size_t size, rank_type dst, tag_type tag);
    request recv(detail::message_buffer& msg, std::size_t size, rank_type src, tag_type tag);

    void send(detail::message_buffer&& msg, std::size_t size, rank_type dst, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    void recv(detail::message_buffer&& msg, std::size_t size, rank_type src, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    detail::message_buffer clone_buffer(detail::message_buffer& msg);

    //impl* get_impl();
};

} // namespace oomph
