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

#include <boost/mp11.hpp>

namespace oomph
{
namespace tensor
{
template<std::size_t... I>
struct layout
{
    static constexpr std::size_t N = sizeof...(I);
    static constexpr std::size_t max_arg = N - 1;

    using arg_list = boost::mp11::mp_list<boost::mp11::mp_size_t<I>...>;
    template<typename A, typename B>
    using sorter = boost::mp11::mp_bool<(A::value < B::value)>;
    using sorted_arg_list = boost::mp11::mp_sort<arg_list, sorter>;
    using lookup_list = boost::mp11::mp_iota_c<N>;
    static_assert(std::is_same<sorted_arg_list, boost::mp11::mp_iota_c<N>>::value,
        "arguments must be unique, contiguous and starting from 0");
    template<typename A>
    using F = boost::mp11::mp_find<arg_list, A>;
    using reverse_lookup = boost::mp11::mp_transform<F, lookup_list>;

    // Get the position of the element with value `i` in the layout
    static constexpr std::size_t find(std::size_t i)
    {
        return find_impl(i, boost::mp11::make_index_sequence<N>{});
    }

    // Get the value of the element at position `i` in the layout
    static constexpr std::size_t at(std::size_t i)
    {
        std::size_t const ri[] = {I...};
        return ri[i];
    }

    template<std::size_t... J>
    static constexpr std::size_t find_impl(std::size_t i, boost::mp11::index_sequence<J...>)
    {
        std::size_t const ri[] = {boost::mp11::mp_at_c<reverse_lookup, J>::value...};
        return ri[i];
    }
};

} // namespace tensor
} // namespace oomph
