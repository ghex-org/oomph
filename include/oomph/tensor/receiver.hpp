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

#include <oomph/tensor/map.hpp>
#include <oomph/tensor/buffer_cache.hpp>
#include <oomph/tensor/detail/terminal.hpp>

namespace oomph
{
namespace tensor
{
template<typename Tensor>
class receiver;

template<typename T, typename Layout>
class receiver<map<T, Layout>> : private detail::terminal<detail::map<T, Layout>>
{
  private:
    using base = detail::terminal<detail::map<T, Layout>>;

  public:
    using map_type = map<T, Layout>;
    using range_type = typename base::range_type;
    using pack_handle = typename base::pack_handle;
    using handle = typename base::handle;

  public:
    receiver(map_type& m)
    : base(m)
    {
    }

    receiver(map_type& m, buffer_cache<T> const& c)
    : base(m, c.m_cache)
    {
    }

    receiver(receiver&&) noexcept = default;
    receiver& operator=(receiver&&) noexcept = default;

    receiver& add_src(range_type const& view, int rank, int tag, std::size_t stage = 0)
    {
        base::add_range(view, rank, tag, stage);
        return *this;
    }

    receiver& connect()
    {
        base::connect();
        return *this;
    }

    pack_handle unpack(std::size_t stage = 0) { return base::unpack(stage); }

    handle recv(std::size_t stage = 0) { return base::recv(stage); }
};

template<typename T, typename Layout>
receiver<map<T, Layout>>
make_receiver(map<T, Layout>& m)
{
    return {m};
}

template<typename T, typename Layout>
receiver<map<T, Layout>>
make_receiver(map<T, Layout>& m, buffer_cache<T> const& c)
{
    return {m, c};
}
} // namespace tensor
} // namespace oomph
