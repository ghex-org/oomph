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
#include <oomph/tensor/detail/map.hpp>

namespace oomph
{
namespace tensor
{
namespace detail
{
template<typename Tensor>
class terminal;
} // namespace detail

template<typename T, typename Layout>
class map : public detail::map<T, Layout>
{
  private:
    friend class oomph::context;
    friend class detail::terminal<detail::map<T, Layout>>;
    using base = detail::map<T, Layout>;

  public:
    using base::dim;
    using vec = typename base::vec;

  private:
    context_impl* m_context;

  private:
    map(context_impl* c, vec const& extents, T* first, T* last)
    : base(extents, first, last)
    , m_context{c}
    {
    }

  public:
    map(map const&) = delete;
    map& operator=(map const&) = delete;

    map(map&& other) noexcept
    : base(std::move(other))
    , m_context{std::exchange(other.m_context, nullptr)}
    {
    }

    map& operator=(map&& other) noexcept
    {
        deregister();
        static_cast<base&>(*this) = std::move(other);
        m_context = std::exchange(other.m_context, nullptr);
    }

    ~map() { deregister(); }

  private:
    void deregister()
    {
        if (m_context) {}
    }
};

} // namespace tensor

template<typename Layout, typename T>
tensor::map<T, Layout>
context::map_tensor(tensor::vector<std::size_t, Layout::max_arg + 1> const& extents, T* first,
    T* last)
{
    return {m.get(), extents, first, last};
}
} // namespace oomph
