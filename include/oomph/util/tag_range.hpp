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

#include <cassert>
#include <oomph/config.hpp>

namespace oomph
{
namespace util
{

class tag_range_factory;

class wrapped_tag
{
  private:
    friend class tag_range;

  private:
    unsigned int m_value;
    unsigned int m_mask;

  private:
    wrapped_tag(unsigned int value, unsigned int mask)
    : m_value{value}
    , m_mask{mask}
    {
    }

  public:
    wrapped_tag(wrapped_tag const&) noexcept = default;
    wrapped_tag& operator=(wrapped_tag const&) noexcept = default;

  public:
    tag_type get() const noexcept { return m_value; }
    tag_type unwrap() const noexcept { return m_value & m_mask; }
};

class tag_range
{
  private:
    friend class tag_range_factory;

  private:
    unsigned int m_range_mask;
    unsigned int m_max_tag;

  private:
    tag_range(unsigned int range_mask, unsigned int max_tag) noexcept
    : m_range_mask{range_mask}
    , m_max_tag{max_tag}
    {
    }

  public:
    tag_range(tag_range const&) noexcept = default;
    tag_range(tag_range&&) noexcept = default;

    wrapped_tag wrap(tag_type t) const noexcept
    {
        assert((t >= 0) && "tag must not be negative");
        assert(((unsigned int)t < m_max_tag) && "tag is too large");
        return {(unsigned int)t | m_range_mask, m_max_tag - 1};
    }

    tag_type max_tag() const noexcept { return m_max_tag; }
};

// tag:
// x|r...|t....|
// x: reserved bit
// r: range bits
// t: tag bits
class tag_range_factory
{
  private:
    unsigned int m_n_ranges;      // number of ranges
    unsigned int m_n_bits;        // number of bits without reserved bit
    unsigned int m_n_range_bits;  // number of range bits
    unsigned int m_reserved_mask; // mask for reserved bit

  public:
    tag_range_factory(unsigned int n_ranges, unsigned int n_bits) noexcept
    : m_n_ranges{n_ranges ? n_ranges : 1}
    , m_n_bits{n_bits - 1}
    , m_n_range_bits{0}
    , m_reserved_mask{1u << m_n_bits}
    {
        assert((n_bits > 1) && "not eneough available bits");
        n_ranges = m_n_ranges - 1;
        while (n_ranges)
        {
            n_ranges >>= 1;
            ++m_n_range_bits;
        }
        assert((m_n_bits > m_n_range_bits) && "not enough available bits");
    }

    tag_range_factory(tag_range_factory const&) noexcept = default;
    tag_range_factory(tag_range_factory&&) noexcept = default;
    tag_range_factory& operator=(tag_range_factory const&) noexcept = default;
    tag_range_factory& operator=(tag_range_factory&&) noexcept = default;

  public:
    tag_range create(unsigned int n, bool reserved = false) const noexcept
    {
        assert((n < m_n_ranges) && "range violation");
        return {(n << (m_n_bits - m_n_range_bits)) | (reserved ? m_reserved_mask : 0u),
            1u << (m_n_bits - m_n_range_bits)};
    }

    unsigned int num_ranges() const noexcept { return m_n_ranges; }
};

} // namespace util
} // namespace oomph
