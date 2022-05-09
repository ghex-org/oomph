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
    using shared_request_ptr = detail::shared_request_ptr;

  public:
    static constexpr rank_type any_source = -1;
    static constexpr tag_type  any_tag = -1;

  private:
    friend class context;
    friend class send_channel_base;
    friend class recv_channel_base;

  public:
    struct schedule
    {
        std::size_t scheduled_sends = 0;
        std::size_t scheduled_recvs = 0;
    };

  private:
    struct cb_none
    {
        shared_request_ptr req;

        void operator()() noexcept
        {
            req->m_ready = true;
            --(*(req->m_scheduled));
        }
    };

    template<typename T, typename CallBack>
    struct cb_rref
    {
        shared_request_ptr req;
        message_buffer<T>  m;
        rank_type          r;
        tag_type           t;
        CallBack           cb;

        void operator()() noexcept
        {
            cb(std::move(m), r, t);
            req->m_ready = true;
            --(*(req->m_scheduled));
        }
    };

    template<typename T, typename CallBack>
    struct cb_lref
    {
        shared_request_ptr req;
        message_buffer<T>* m;
        rank_type          r;
        tag_type           t;
        CallBack           cb;

        void operator()() noexcept
        {
            cb(*m, r, t);
            req->m_ready = true;
            --(*(req->m_scheduled));
        }
    };

    template<typename T, typename CallBack>
    struct cb_lref_const
    {
        shared_request_ptr       req;
        message_buffer<T> const* m;
        rank_type                r;
        tag_type                 t;
        CallBack                 cb;

        void operator()() noexcept
        {
            cb(*m, r, t);
            req->m_ready = true;
            --(*(req->m_scheduled));
        }
    };

    template<typename Message>
    struct msg_ref_count
    {
        Message                msg;
        int                    counter;
        std::vector<rank_type> neighs;
    };

    using pool_type = boost::pool<boost::default_user_allocator_malloc_free>;

  private:
    impl_type*                 m_impl;
    std::unique_ptr<pool_type> m_pool;
    std::unique_ptr<pool_type> m_send_multi_pool_1;
    std::unique_ptr<pool_type> m_send_multi_pool_2;
    std::unique_ptr<pool_type> m_send_multi_pool_3;
    std::unique_ptr<schedule>  m_schedule;

  private:
    communicator(impl_type* impl_);

  public:
    communicator(communicator const&) = delete;

    communicator(communicator&& other) noexcept
    : m_impl{std::exchange(other.m_impl, nullptr)}
    , m_pool{std::move(other.m_pool)}
    , m_send_multi_pool_1{std::move(other.m_send_multi_pool_1)}
    , m_send_multi_pool_2{std::move(other.m_send_multi_pool_2)}
    , m_schedule{std::move(other.m_schedule)}
    {
    }

    communicator& operator=(communicator const&) = delete;

    communicator& operator=(communicator&& other) noexcept
    {
        m_impl = std::exchange(other.m_impl, nullptr);
        m_pool = std::move(other.m_pool);
        m_send_multi_pool_1 = std::move(other.m_send_multi_pool_1);
        m_send_multi_pool_2 = std::move(other.m_send_multi_pool_2);
        m_schedule = std::move(other.m_schedule);
        return *this;
    }

    ~communicator();

  public:
    rank_type   rank() const noexcept;
    rank_type   size() const noexcept;
    MPI_Comm    mpi_comm() const noexcept;
    bool        is_local(rank_type rank) const noexcept;
    std::size_t scheduled_sends() const noexcept { return m_schedule->scheduled_sends; }
    std::size_t scheduled_recvs() const noexcept { return m_schedule->scheduled_recvs; }
    bool is_ready() const noexcept { return (scheduled_sends() == 0) && (scheduled_recvs() == 0); }

    void wait_all()
    {
        while (!is_ready()) { progress(); }
    }

    template<typename T>
    message_buffer<T> make_buffer(std::size_t size)
    {
        return {make_buffer_core(size * sizeof(T)), size};
    }

    template<typename T>
    message_buffer<T> make_buffer(T* ptr, std::size_t size)
    {
        return {make_buffer_core(ptr, size * sizeof(T)), size};
    }

#if HWMALLOC_ENABLE_DEVICE
    template<typename T>
    message_buffer<T> make_device_buffer(std::size_t size, int id = hwmalloc::get_device_id())
    {
        return {make_buffer_core(size * sizeof(T), id), size};
    }

    template<typename T>
    message_buffer<T> make_device_buffer(T* device_ptr, std::size_t size,
        int id = hwmalloc::get_device_id())
    {
        return {make_buffer_core(device_ptr, size * sizeof(T), id), size};
    }

    template<typename T>
    message_buffer<T> make_device_buffer(T* ptr, T* device_ptr, std::size_t size,
        int id = hwmalloc::get_device_id())
    {
        return {make_buffer_core(ptr, device_ptr, size * sizeof(T), id), size};
    }
#endif

    // no callback versions
    // ====================

    template<typename T>
    [[nodiscard]] recv_request recv(message_buffer<T>& msg, rank_type src, tag_type tag)
    {
        assert(msg);
        auto& scheduled = m_schedule->scheduled_recvs;
        ++scheduled;
        recv_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
        recv(msg.m.m_heap_ptr.get(), msg.size() * sizeof(T), src, tag, cb_none{r.m_data}, r.m_data);
        return r;
    }

    template<typename T>
    [[nodiscard]] send_request send(message_buffer<T> const& msg, rank_type dst, tag_type tag)
    {
        assert(msg);
        auto& scheduled = m_schedule->scheduled_sends;
        ++scheduled;
        send_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
        send(msg.m.m_heap_ptr.get(), msg.size() * sizeof(T), dst, tag, cb_none{r.m_data}, r.m_data);
        return r;
    }

    template<typename T>
    send_request send_multi(message_buffer<T> const& msg, rank_type const* neighs,
        std::size_t neighs_size, tag_type tag)
    {
        assert(msg);
        auto& scheduled = m_schedule->scheduled_sends;
        scheduled += neighs_size;
        send_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));

        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       p = m_send_multi_pool_3.get();
        auto       counter = ::new (p->malloc()) int{(int)neighs_size};

        for (std::size_t i = 0; i < neighs_size; ++i)
        {
            rank_type const id = neighs[i];
            send_request    rx(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
            send(
                m_ptr, s * sizeof(T), id, tag,
                [rd = r.m_data, rdx = rx.m_data, counter, p]() mutable
                {
                    if ((--(*counter)) == 0)
                    {
                        p->free(counter);
                        rd->m_ready = true;
                    }
                    --(*(rd->m_scheduled));
                },
                rx.m_data);
        }
        return r;
    }

    template<typename T>
    send_request send_multi(message_buffer<T> const& msg, std::vector<rank_type> const& neighs,
        tag_type tag)
    {
        return send_multi(msg, neighs.data(), neighs.size(), tag);
    }

    // callback versions
    // =================

    template<typename T, typename CallBack>
    recv_request recv(message_buffer<T>&& msg, rank_type src, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        assert(msg);
        auto& scheduled = m_schedule->scheduled_recvs;
        ++scheduled;
        recv_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        recv(m_ptr, s * sizeof(T), src, tag,
            cb_rref<T, std::decay_t<CallBack>>{r.m_data, std::move(msg), src, tag,
                std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    recv_request recv(message_buffer<T>& msg, rank_type src, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_REF(CallBack)
        assert(msg);
        auto& scheduled = m_schedule->scheduled_recvs;
        ++scheduled;
        recv_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        recv(m_ptr, s * sizeof(T), src, tag,
            cb_lref<T, std::decay_t<CallBack>>{r.m_data, &msg, src, tag,
                std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    send_request send(message_buffer<T>&& msg, rank_type dst, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        assert(msg);
        auto& scheduled = m_schedule->scheduled_sends;
        ++scheduled;
        send_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        send(m_ptr, s * sizeof(T), dst, tag,
            cb_rref<T, std::decay_t<CallBack>>{r.m_data, std::move(msg), dst, tag,
                std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    send_request send(message_buffer<T>& msg, rank_type dst, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_REF(CallBack)
        assert(msg);
        auto& scheduled = m_schedule->scheduled_sends;
        ++scheduled;
        send_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        send(m_ptr, s * sizeof(T), dst, tag,
            cb_lref<T, std::decay_t<CallBack>>{r.m_data, &msg, dst, tag,
                std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    send_request send(message_buffer<T> const& msg, rank_type dst, tag_type tag,
        CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_CONST_REF(CallBack)
        assert(msg);
        auto& scheduled = m_schedule->scheduled_sends;
        ++scheduled;
        send_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
        const auto   s = msg.size();
        auto         m_ptr = msg.m.m_heap_ptr.get();

        send(m_ptr, s * sizeof(T), dst, tag,
            cb_lref_const<T, std::decay_t<CallBack>>{r.m_data, &msg, dst, tag,
                std::forward<CallBack>(callback)},
            r.m_data);
        return r;
    }

    template<typename T, typename CallBack>
    send_request send_multi(message_buffer<T>&& msg, std::vector<rank_type> neighs, tag_type tag,
        CallBack&& callback)
    {
        using msg_ref_count_t = msg_ref_count<message_buffer<T>>;

        OOMPH_CHECK_CALLBACK_MULTI(CallBack)
        assert(msg);

        int const  ns = neighs.size();
        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       p = m_send_multi_pool_2.get();
        auto       m = ::new (p->malloc()) msg_ref_count_t{std::move(msg), ns, std::move(neighs)};

        auto& scheduled = m_schedule->scheduled_sends;
        scheduled += neighs.size();
        send_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));

        for (auto id : m->neighs)
        {
            send_request rx(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
            send(
                m_ptr, s * sizeof(T), id, tag,
                [rd = r.m_data, rdx = rx.m_data, m, p, tag,
                    cb = std::forward<CallBack>(callback)]() mutable
                {
                    if ((--(m->counter)) == 0)
                    {
                        cb(std::move(m->msg), std::move(m->neighs), tag);
                        m->~msg_ref_count_t();
                        p->free(m);
                        rd->m_ready = true;
                    }
                    --(*(rd->m_scheduled));
                },
                rx.m_data);
        }
        return r;
    }

    template<typename T, typename CallBack>
    send_request send_multi(message_buffer<T>& msg, std::vector<rank_type> neighs, tag_type tag,
        CallBack&& callback)
    {
        using msg_ref_count_t = msg_ref_count<message_buffer<T>*>;

        OOMPH_CHECK_CALLBACK_MULTI_REF(CallBack)
        assert(msg);

        int const  ns = neighs.size();
        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       p = m_send_multi_pool_1.get();
        auto       m = ::new (p->malloc()) msg_ref_count_t{&msg, ns, std::move(neighs)};

        return send_multi_ref_impl(m_ptr, m, s * sizeof(T), tag, std::forward<CallBack>(callback),
            p);
    }

    template<typename T, typename CallBack>
    send_request send_multi(message_buffer<T> const& msg, std::vector<rank_type> neighs,
        tag_type tag, CallBack&& callback)
    {
        using msg_ref_count_t = msg_ref_count<message_buffer<T> const*>;

        OOMPH_CHECK_CALLBACK_MULTI_CONST_REF(CallBack)
        assert(msg);

        int const  ns = neighs.size();
        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       p = m_send_multi_pool_1.get();
        auto       m = ::new (p->malloc()) msg_ref_count_t{&msg, ns, std::move(neighs)};

        return send_multi_ref_impl(m_ptr, m, s * sizeof(T), tag, std::forward<CallBack>(callback),
            p);
    }

    void progress();

  private:
    detail::message_buffer make_buffer_core(std::size_t size);
    detail::message_buffer make_buffer_core(void* ptr, std::size_t size);
#if HWMALLOC_ENABLE_DEVICE
    detail::message_buffer make_buffer_core(std::size_t size, int device_id);
    detail::message_buffer make_buffer_core(void* device_ptr, std::size_t size, int device_id);
    detail::message_buffer make_buffer_core(void* ptr, void* device_ptr, std::size_t size,
        int device_id);
#endif

    void send(detail::message_buffer::heap_ptr_impl const* m_ptr, std::size_t size, rank_type dst,
        tag_type tag, util::unique_function<void()> cb, shared_request_ptr req);

    void recv(detail::message_buffer::heap_ptr_impl* m_ptr, std::size_t size, rank_type src,
        tag_type tag, util::unique_function<void()> cb, shared_request_ptr req);

    template<typename HeapPtr, typename RefCount, typename CallBack, typename Pool>
    send_request send_multi_ref_impl(HeapPtr* m_ptr, RefCount* m, std::size_t msg_size,
        tag_type tag, CallBack&& callback, Pool* p)
    {
        auto& scheduled = m_schedule->scheduled_sends;
        scheduled += m->neighs.size();
        send_request r(shared_request_ptr(m_pool.get(), m_impl, &scheduled));

        for (auto id : m->neighs)
        {
            send_request rx(shared_request_ptr(m_pool.get(), m_impl, &scheduled));
            send(
                m_ptr, msg_size, id, tag,
                [rd = r.m_data, rdx = rx.m_data, m, p, tag,
                    cb = std::forward<CallBack>(callback)]() mutable
                {
                    if ((--(m->counter)) == 0)
                    {
                        cb(*(m->msg), std::move(m->neighs), tag);
                        m->~RefCount();
                        p->free(m);
                        rd->m_ready = true;
                    }
                    --(*(rd->m_scheduled));
                },
                rx.m_data);
        }
        return r;
    }
};

} // namespace oomph
