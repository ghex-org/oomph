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

#include <atomic>
#include <boost/callable_traits.hpp>
#include <oomph/request.hpp>
#include <oomph/util/pool_factory.hpp>
//#include <oomph/util/tag_range.hpp>

#define OOMPH_CHECK_CALLBACK_F(CALLBACK, RANK_TYPE)                                                \
    using args_t = boost::callable_traits::args_t<std::remove_reference_t<CALLBACK>>;              \
    using arg0_t = std::tuple_element_t<0, args_t>;                                                \
    using arg1_t = std::tuple_element_t<1, args_t>;                                                \
    using arg2_t = std::tuple_element_t<2, args_t>;                                                \
    static_assert(std::tuple_size<args_t>::value == 3, "callback must have 3 arguments");          \
    static_assert(std::is_same<arg1_t, RANK_TYPE>::value,                                          \
        "rank_type is not convertible to second callback argument type");                          \
    static_assert(std::is_same<arg2_t, tag_type>::value,                                           \
        "tag_type is not convertible to third callback argument type");                            \
    using TT = typename std::remove_reference_t<arg0_t>::value_type;

#define OOMPH_CHECK_CALLBACK_MSG                                                                   \
    static_assert(std::is_same<arg0_t, message_buffer<TT>>::value,                                 \
        "first callback argument type is not a message_buffer");

#define OOMPH_CHECK_CALLBACK_MSG_REF                                                               \
    static_assert(std::is_same<arg0_t, message_buffer<TT>&>::value ||                              \
                      std::is_same<arg0_t, message_buffer<TT> const&>::value,                      \
        "first callback argument type is not an l-value reference to a message_buffer");

#define OOMPH_CHECK_CALLBACK_MSG_CONST_REF                                                         \
    static_assert(std::is_same<arg0_t, message_buffer<TT> const&>::value,                          \
        "first callback argument type is not a const l-value reference to a message_buffer");

#define OOMPH_CHECK_CALLBACK(CALLBACK)                                                             \
    {                                                                                              \
        OOMPH_CHECK_CALLBACK_F(CALLBACK, rank_type)                                                \
        OOMPH_CHECK_CALLBACK_MSG                                                                   \
    }

#define OOMPH_CHECK_CALLBACK_MULTI(CALLBACK)                                                       \
    {                                                                                              \
        OOMPH_CHECK_CALLBACK_F(CALLBACK, std::vector<rank_type>)                                   \
        OOMPH_CHECK_CALLBACK_MSG                                                                   \
    }

#define OOMPH_CHECK_CALLBACK_REF(CALLBACK)                                                         \
    {                                                                                              \
        OOMPH_CHECK_CALLBACK_F(CALLBACK, rank_type)                                                \
        OOMPH_CHECK_CALLBACK_MSG_REF                                                               \
    }

#define OOMPH_CHECK_CALLBACK_MULTI_REF(CALLBACK)                                                   \
    {                                                                                              \
        OOMPH_CHECK_CALLBACK_F(CALLBACK, std::vector<rank_type>)                                   \
        OOMPH_CHECK_CALLBACK_MSG_REF                                                               \
    }

#define OOMPH_CHECK_CALLBACK_CONST_REF(CALLBACK)                                                   \
    {                                                                                              \
        OOMPH_CHECK_CALLBACK_F(CALLBACK, rank_type)                                                \
        OOMPH_CHECK_CALLBACK_MSG_CONST_REF                                                         \
    }

#define OOMPH_CHECK_CALLBACK_MULTI_CONST_REF(CALLBACK)                                             \
    {                                                                                              \
        OOMPH_CHECK_CALLBACK_F(CALLBACK, std::vector<rank_type>)                                   \
        OOMPH_CHECK_CALLBACK_MSG_CONST_REF                                                         \
    }

namespace oomph
{
class communicator_impl;

namespace detail
{
struct communicator_state
{
    using impl_type = communicator_impl;
    impl_type*                              m_impl;
    std::atomic<std::size_t>*               m_shared_scheduled_recvs;
    //util::tag_range                         m_tag_range;
    //util::tag_range                         m_reserved_tag_range;
    util::pool_factory<multi_request_state> m_mrs_factory;
    std::size_t                             scheduled_sends = 0;
    std::size_t                             scheduled_recvs = 0;

    communicator_state(impl_type* impl_, std::atomic<std::size_t>* shared_scheduled_recvs);
        //,util::tag_range tr, util::tag_range rtr);
    ~communicator_state();
    communicator_state(communicator_state const&) = delete;
    communicator_state(communicator_state&&) = delete;
    communicator_state& operator=(communicator_state const&) = delete;
    communicator_state& operator=(communicator_state&&) = delete;

    auto make_multi_request_state(std::size_t ns) { return m_mrs_factory.make(m_impl, ns); }

    template<typename T>
    auto make_multi_request_state(std::vector<int>&& neighs, oomph::message_buffer<T> const& msg)
    {
        return m_mrs_factory.make(m_impl, neighs.size(), std::move(neighs), msg.size(), &msg);
    }

    template<typename T>
    auto make_multi_request_state(std::vector<int>&& neighs, oomph::message_buffer<T>& msg)
    {
        return m_mrs_factory.make(m_impl, neighs.size(), std::move(neighs), msg.size(), &msg);
    }

    template<typename T>
    auto make_multi_request_state(std::vector<int>&& neighs, oomph::message_buffer<T>&& msg)
    {
        return m_mrs_factory.make(m_impl, neighs.size(), std::move(neighs), msg.size(), nullptr,
            std::move(msg.m));
    }
};

//struct reserved_tag
//{
//    tag_type m;
//};
} // namespace detail
} // namespace oomph
