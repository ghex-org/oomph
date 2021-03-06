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

#include <boost/callable_traits.hpp>

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
