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

#include <oomph/context.hpp>

namespace oomph
{
namespace detail
{

template<bool>
struct request_state_traits
{
    template<typename T>
    using type = T;

    template<typename T>
    static inline void store(T& dst, T const& v) noexcept
    {
        dst = v;
    }

    template<typename T>
    static inline T load(T const& src) noexcept
    {
        return src;
    }
};

template<>
struct request_state_traits<true>
{
    template<typename T>
    using type = std::atomic<T>;

    template<typename T>
    static inline void store(type<T>& dst, T const& v) noexcept
    {
        dst.store(v);
    }

    template<typename T>
    static inline T load(type<T> const& src) noexcept
    {
        return src.load();
    }
};

template<bool Threadsafe>
struct request_state_base
{
    using traits = request_state_traits<Threadsafe>;
    using context_type = oomph::context_impl;
    using communicator_type = oomph::communicator_impl;
    using cb_type = oomph::util::unique_function<void(rank_type, tag_type)>;

    template<typename T>
    using type = typename traits::template type<T>;

    context_type*      m_ctxt;
    communicator_type* m_comm;
    type<std::size_t>* m_scheduled;
    rank_type          m_rank;
    tag_type           m_tag;
    cb_type            m_cb;
    type<bool>         m_ready;
    type<bool>         m_canceled;

    request_state_base(context_type* ctxt, communicator_type* comm, type<std::size_t>* scheduled,
        rank_type rank, tag_type tag, cb_type&& cb)
    : m_ctxt{ctxt}
    , m_comm{comm}
    , m_scheduled{scheduled}
    , m_rank{rank}
    , m_tag{tag}
    , m_cb{std::move(cb)}
    , m_ready(false)
    , m_canceled(false)
    {
        ++(*m_scheduled);
    }

    bool is_ready() const noexcept { return traits::load(m_ready); }

    bool is_canceled() const noexcept { return traits::load(m_canceled); }

    void invoke_cb()
    {
        m_cb(m_rank, m_tag);
        --(*m_scheduled);
        traits::store(m_ready, true);
    }

    void set_canceled()
    {
        --(*m_scheduled);
        traits::store(m_ready, true);
        traits::store(m_canceled, true);
    }
};

} // namespace detail

} // namespace oomph
