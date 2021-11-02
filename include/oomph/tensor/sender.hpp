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

#include <oomph/tensor/detail/terminal.hpp>
#include <oomph/tensor/map.hpp>

namespace oomph
{
namespace tensor
{
template<typename Tensor>
class sender;

template<typename T, typename Layout>
class sender<map<T, Layout>> : private detail::terminal<detail::map<T, Layout>>
{
  private:
    using base = detail::terminal<detail::map<T, Layout>>;

  public:
    using map_type = map<T, Layout>;
    using range_type = typename base::range_type;
    using pack_handle = typename base::pack_handle;
    using handle = typename base::handle;

  public:
    sender(map_type& m)
    : base(m)
    {
    }

    sender(sender&&) noexcept = default;
    sender& operator=(sender&&) noexcept = default;

    sender& add_dst(range_type const& view, int rank, int tag)
    {
        base::add_range(view, rank, tag);
        return *this;
    }

    sender& connect()
    {
        base::connect();
        return *this;
    }

    pack_handle pack() { return base::pack(); }

    handle send() { return base::send(); }
};

template<typename T, typename Layout>
sender<map<T, Layout>>
make_sender(map<T, Layout>& m)
{
    return {m};
}

} // namespace tensor
} // namespace oomph
