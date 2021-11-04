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

#include <vector>

namespace oomph
{
namespace tensor
{
class rt_layout
{
  private:
    std::vector<std::size_t> const m;

  public:
    rt_layout(std::initializer_list<std::size_t> l)
    : m{l}
    {
    }

    rt_layout(std::size_t* first, std::size_t* last)
    : m(first, last)
    {
    }

    rt_layout(std::size_t* first, std::size_t count)
    : rt_layout(first, first + count)
    {
    }

    rt_layout(rt_layout const&) = default;

    rt_layout(rt_layout&&) noexcept = default;

  public:
    friend bool operator==(rt_layout const& a, rt_layout const& b)
    {
        std::size_t const s_a = a.size();
        std::size_t const s_b = b.size();
        if (s_a != s_b) return false;
        for (std::size_t i = 0; i < s_a; ++i)
            if (a.at(i) != b.at(i)) return false;
        return true;
    }

    template<typename Layout>
    bool is_equal() const noexcept
    {
        std::size_t const s = m.size();
        if (Layout::max_arg + 1 != s) return false;
        for (std::size_t i = 0; i < s; ++i)
            if (Layout::at(i) != m[i]) return false;
        return true;
    }

  public:
    std::size_t const* data() const noexcept { return m.data(); }
    std::size_t        size() const noexcept { return m.size(); }

  public:
    // Get the position of the element with value `i` in the layout
    std::size_t find(std::size_t i) const noexcept
    {
        std::size_t const s = m.size();
        for (std::size_t j = 0; j < s; ++j)
            if (m[j] == i) return j;
        return s;
    }

    // Get the value of the element at position `i` in the layout
    std::size_t at(std::size_t i) const noexcept { return m[i]; }
};

namespace detail
{
template<typename Layout, std::size_t... Is>
rt_layout make_rt_layout(std::index_sequence<Is...>) noexcept
{
    return {{Layout::at(Is)...}};
}

} // namespace detail

template<typename Layout>
rt_layout
make_rt_layout() noexcept
{
    return detail::make_rt_layout<Layout>(std::make_index_sequence<Layout::max_arg + 1>{});
}

} // namespace tensor
} // namespace oomph
