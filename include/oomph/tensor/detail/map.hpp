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

#include <oomph/tensor/vector.hpp>

namespace oomph
{
namespace tensor
{
namespace detail
{
template<typename T, typename Layout>
class map
{
  public:
    static constexpr std::size_t dim() noexcept { return Layout::max_arg + 1; };

    using vec = vector<std::size_t, dim()>;

  protected:
    static constexpr std::size_t s_stride_1_dim = Layout::find(dim() - 1);

  protected:
    T*          m_data;
    std::size_t m_num_elements;
    vec         m_extents;
    vec         m_strides;
    std::size_t m_line_size;

  public:
    map(vec const& extents, T* first, T* last)
    : m_data{first}
    , m_num_elements{product(extents)}
    , m_extents{extents}
    {
        auto const        stride_1_extent = m_extents[s_stride_1_dim];
        auto const        num_lines = m_num_elements / stride_1_extent;
        auto const        total_padding = (((last + 1) - first) - m_num_elements) * sizeof(T);
        std::size_t const padding = (num_lines == 1) ? 0 : total_padding / (num_lines - 1);
        m_strides[s_stride_1_dim] = 1;
        std::size_t s = stride_1_extent * sizeof(T) + padding;
        assert((s / sizeof(T)) * sizeof(T) == s);
        s /= sizeof(T);
        m_line_size = s;
        for (std::size_t i = 1; i < dim(); ++i)
        {
            m_strides[Layout::find(dim() - 1 - i)] = s;
            s *= m_extents[Layout::find(dim() - 1 - i)];
        }
    }

    map(map const&) noexcept = default;
    map(map&&) noexcept = default;
    map& operator=(map const&) noexcept = default;
    map& operator=(map&&) noexcept = default;

  public:
    auto const& strides() const noexcept { return m_strides; }
    vec const&  extents() const noexcept { return m_extents; }
    std::size_t line_size() const noexcept { return m_line_size; }
    T*          get_address(vec coord) const noexcept { return m_data + dot(coord, m_strides); }
};

} // namespace detail
} // namespace tensor
} // namespace oomph
