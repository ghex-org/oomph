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
#include <oomph/detail/communicator_helper.hpp>
#include <oomph/util/unique_function.hpp>
#include <hwmalloc/device.hpp>
#include <functional>
#include <vector>
#include <atomic>
#include <cassert>
#include <boost/callable_traits.hpp>

namespace oomph
{
class context;
class send_channel_base;
class recv_channel_base;

class communicator_impl;

class communicator
{
  public:
    using rank_type = int;
    using tag_type = int;
    using impl_type = communicator_impl;

  public:
    static constexpr rank_type any_source = -1;
    static constexpr tag_type  any_tag = -1;

  private:
    friend class context;
    friend class send_channel_base;
    friend class recv_channel_base;

  private:
    impl_type* m_impl;

  private:
    using shared_req_ptr = std::shared_ptr<detail::request_state>;

    struct cb_none
    {
        shared_req_ptr req;

        void operator()() noexcept { req->m_ready = true; }
    };

    template<typename T, typename CallBack>
    struct cb_rref
    {
        shared_req_ptr    req;
        message_buffer<T> m;
        rank_type         r;
        tag_type          t;
        CallBack          cb;

        void operator()() noexcept
        {
            cb(std::move(m), r, t);
            req->m_ready = true;
        }
    };

    template<typename T, typename CallBack>
    struct cb_lref
    {
        shared_req_ptr     req;
        message_buffer<T>* m;
        rank_type          r;
        tag_type           t;
        CallBack           cb;

        void operator()() noexcept
        {
            cb(*m, r, t);
            req->m_ready = true;
        }
    };

    template<typename T, typename CallBack>
    struct cb_lref_const
    {
        shared_req_ptr           req;
        message_buffer<T> const* m;
        rank_type                r;
        tag_type                 t;
        CallBack                 cb;

        void operator()() noexcept
        {
            cb(*m, r, t);
            req->m_ready = true;
        }
    };

  private:
    communicator(impl_type* impl_) noexcept
    : m_impl{impl_}
    {
    }

  public:
    communicator(communicator const&) = delete;

    communicator(communicator&& other) noexcept
    : m_impl{std::exchange(other.m_impl, nullptr)}
    {
    }

    communicator& operator=(communicator const&) = delete;

    communicator& operator=(communicator&& other) noexcept
    {
        m_impl = std::exchange(other.m_impl, nullptr);
        return *this;
    }

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

#if HWMALLOC_ENABLE_DEVICE
    template<typename T>
    message_buffer<T> make_device_buffer(std::size_t size, int id = hwmalloc::get_device_id())
    {
        return {make_buffer_core(size * sizeof(T)), size, id};
    }
#endif

    // no callback versions
    // ====================

    template<typename T>
    [[nodiscard]] recv_request recv(message_buffer<T>& msg, rank_type src, tag_type tag)
    {
        assert(msg);
        recv_request r(std::make_shared<recv_request::data_type>(m_impl));
        recv(msg.m.m_heap_ptr.get(), msg.size() * sizeof(T), src, tag, cb_none{r.m_data}, r.m_data);
        return r;
    }

    template<typename T>
    [[nodiscard]] send_request send(message_buffer<T> const& msg, rank_type dst, tag_type tag)
    {
        assert(msg);
        send_request r(std::make_shared<send_request::data_type>(m_impl));
        send(msg.m.m_heap_ptr.get(), msg.size() * sizeof(T), dst, tag, cb_none{r.m_data}, r.m_data);
        return r;
    }

    template<typename T>
    [[nodiscard]] send_request send_multi(
        message_buffer<T> const& msg, std::vector<rank_type> const& neighs, tag_type tag)
    {
        assert(msg);
        send_request r(std::make_shared<send_request::data_type>(m_impl));

        const auto s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       counter = new std::atomic<int>{(int)neighs.size()};

        for (auto id : neighs)
        {
            send(
                m_ptr, s * sizeof(T), id, tag,
                [rd = r.m_data, counter]() mutable {
                    if ((--(*counter)) == 0)
                    {
                        delete counter;
                        rd->m_ready = true;
                    }
                },
                r.m_data);
        }
        return r;
    }

    // callback versions
    // =================

    template<typename T, typename CallBack>
    recv_request recv(message_buffer<T>&& msg, rank_type src, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        assert(msg);

        recv_request r(std::make_shared<recv_request::data_type>(m_impl));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        recv(m_ptr, s * sizeof(T), src, tag,
            cb_rref<T, std::decay_t<CallBack>>{
                r.m_data, std::move(msg), src, tag, std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    recv_request recv(message_buffer<T>& msg, rank_type src, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_REF(CallBack)
        assert(msg);

        recv_request r(std::make_shared<recv_request::data_type>(m_impl));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        recv(m_ptr, s * sizeof(T), src, tag,
            cb_lref<T, std::decay_t<CallBack>>{
                r.m_data, &msg, src, tag, std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    send_request send(message_buffer<T>&& msg, rank_type dst, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        assert(msg);

        send_request r(std::make_shared<send_request::data_type>(m_impl));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        send(m_ptr, s * sizeof(T), dst, tag,
            cb_rref<T, std::decay_t<CallBack>>{
                r.m_data, std::move(msg), dst, tag, std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    send_request send(message_buffer<T>& msg, rank_type dst, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_REF(CallBack)
        assert(msg);

        send_request r(std::make_shared<send_request::data_type>(m_impl));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        send(m_ptr, s * sizeof(T), dst, tag,
            cb_lref<T, std::decay_t<CallBack>>{
                r.m_data, &msg, dst, tag, std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    send_request send(
        message_buffer<T> const& msg, rank_type dst, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_CONST_REF(CallBack)
        assert(msg);

        send_request r(std::make_shared<send_request::data_type>(m_impl));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        send(m_ptr, s * sizeof(T), dst, tag,
            cb_lref_const<T, std::decay_t<CallBack>>{
                r.m_data, &msg, dst, tag, std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    send_request send_multi(message_buffer<T>&& msg, std::vector<rank_type> const& neighs,
        tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI(CallBack)
        assert(msg);
        struct msg_ref_count
        {
            message_buffer<T>      msg;
            std::atomic<int>       counter;
            std::vector<rank_type> neighs;
        };

        send_request r(std::make_shared<send_request::data_type>(m_impl));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();
        auto         m = new msg_ref_count{std::move(msg), {(int)neighs.size()}, neighs};

        for (auto id : neighs)
        {
            send(
                m_ptr, s * sizeof(T), id, tag,
                [rd = r.m_data, m, tag, cb = std::forward<CallBack>(callback)]() mutable {
                    if ((--(m->counter)) == 0)
                    {
                        cb(std::move(m->msg), std::move(m->neighs), tag);
                        delete m;
                        rd->m_ready = true;
                    }
                },
                r.m_data);
        }
        return r;
    }

    template<typename T, typename CallBack>
    send_request send_multi(message_buffer<T>& msg, std::vector<rank_type> const& neighs,
        tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI_REF(CallBack)
        assert(msg);
        struct msg_ref_count
        {
            message_buffer<T>*     msg;
            std::atomic<int>       counter;
            std::vector<rank_type> neighs;
        };

        send_request r(std::make_shared<send_request::data_type>(m_impl));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();
        auto         m = new msg_ref_count{&msg, {(int)neighs.size()}, neighs};

        for (auto id : neighs)
        {
            send(
                m_ptr, s * sizeof(T), id, tag,
                [rd = r.m_data, m, tag, cb = std::forward<CallBack>(callback)]() mutable {
                    if ((--(m->counter)) == 0)
                    {
                        cb(*(m->msg), std::move(m->neighs), tag);
                        delete m;
                        rd->m_ready = true;
                    }
                },
                r.m_data);
        }
        return r;
    }

    template<typename T, typename CallBack>
    send_request send_multi(message_buffer<T> const& msg, std::vector<rank_type> const& neighs,
        tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI_CONST_REF(CallBack)
        assert(msg);
        struct msg_ref_count
        {
            message_buffer<T> const* msg;
            std::atomic<int>         counter;
            std::vector<rank_type>   neighs;
        };

        send_request r(std::make_shared<send_request::data_type>(m_impl));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();
        auto         m = new msg_ref_count{&msg, {(int)neighs.size()}, neighs};

        for (auto id : neighs)
        {
            send(
                m_ptr, s * sizeof(T), id, tag,
                [rd = r.m_data, m, tag, cb = std::forward<CallBack>(callback)]() mutable {
                    if ((--(m->counter)) == 0)
                    {
                        cb(*(m->msg), std::move(m->neighs), tag);
                        delete m;
                        rd->m_ready = true;
                    }
                },
                r.m_data);
        }
        return r;
    }

    void progress();

  private:
    detail::message_buffer make_buffer_core(std::size_t size);
#if HWMALLOC_ENABLE_DEVICE
    detail::message_buffer make_buffer_core(std::size_t size, int device_id);
#endif

    void send(detail::message_buffer::heap_ptr_impl const* m_ptr, std::size_t size, rank_type dst,
        tag_type tag, util::unique_function<void()> cb, shared_req_ptr req);

    void recv(detail::message_buffer::heap_ptr_impl* m_ptr, std::size_t size, rank_type src,
        tag_type tag, util::unique_function<void()> cb, shared_req_ptr req);

    bool cancel_recv_cb_(rank_type src, tag_type tag,
        std::function<void(detail::message_buffer, std::size_t size, rank_type, tag_type)>&& cb);
};

} // namespace oomph
