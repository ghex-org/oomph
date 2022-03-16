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
template<std::size_t N>
class range
{
  public:
    using vec = vector<std::size_t, N>;
    //template<typename T, typename Layout>
    //friend class oomph::detail::map;

  private:
    vec m_first;
    vec m_extents;
    vec m_increments;

  public:
    constexpr range(vec const& first, vec const& extents,
        vec const& increments = make_uniform<N>((std::size_t)1))
    : m_first{first}
    , m_extents{extents}
    , m_increments{increments}
    {
    }

    constexpr range(range const&) noexcept = default;
    range& operator=(range const&) noexcept = default;

    vec const& first() const noexcept { return m_first; }
    vec const& extents() const noexcept { return m_extents; }
    vec const& increments() const noexcept { return m_increments; }
};
} // namespace tensor
} // namespace oomph
