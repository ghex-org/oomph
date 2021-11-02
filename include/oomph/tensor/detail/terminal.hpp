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

#include <oomph/communicator.hpp>
#include <oomph/tensor/range.hpp>
#include <oomph/tensor/detail/map.hpp>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

namespace oomph
{
namespace tensor
{
namespace detail
{
template<typename Tensor>
class terminal;

template<typename T, typename Layout>
class terminal<map<T, Layout>>
{
  public:
    using map_type = map<T, Layout>;
    static constexpr std::size_t dim() noexcept { return map_type::dim(); };
    using vec = vector<std::size_t, dim()>;
    using range_type = range<dim()>;

    static constexpr std::size_t last_dim = Layout::find(0);

  protected:
    struct transport_range
    {
        range_type                         m_range;
        std::shared_ptr<message_buffer<T>> m_message;
        int                                m_rank;
        int                                m_tag;
        bool                               m_direct;
    };

    struct serialization_range
    {
        range_type m_range;
        T*         m_ptr;
    };

    struct buffer_cache
    {
        communicator*                                   m_comm;
        std::vector<std::shared_ptr<message_buffer<T>>> m_messages;
        std::map<std::size_t, std::set<std::size_t>>    m_available_idx;

        buffer_cache(communicator* c) noexcept
        : m_comm{c}
        {
        }

        buffer_cache(buffer_cache&&) noexcept = default;
        buffer_cache& operator=(buffer_cache&&) noexcept = default;

        std::shared_ptr<message_buffer<T>> operator()(std::size_t size, std::size_t stage)
        {
            std::set<std::size_t>* index_set = nullptr;
            auto                   it = m_available_idx.find(stage);
            if (it == m_available_idx.end())
            {
                index_set = &(m_available_idx[stage]);
                for (std::size_t i = 0; i < m_messages.size(); ++i) index_set->insert(i);
            }
            else
            {
                index_set = &(it->second);
            }

            auto m_it = std::find_if(index_set->begin(), index_set->end(),
                [this, size](std::size_t i) { return (m_messages[i]->size() == size); });

            if (m_it == index_set->end())
            {
                m_messages.push_back(
                    std::make_shared<message_buffer<T>>(m_comm->make_buffer<T>(size)));
                for (auto& s : m_available_idx) s.second.insert(m_messages.size() - 1);
                index_set->erase(m_messages.size() - 1);
                return m_messages.back();
            }
            else
            {
                auto res = *m_it;
                index_set->erase(m_it);
                return m_messages[res];
            }
        }
    };

    struct stage_t
    {
        std::vector<transport_range>     m_transport_ranges;
        std::vector<serialization_range> m_serialization_ranges;
    };

  protected:
    map_type                       m_map;
    std::unique_ptr<communicator>  m_comm;
    std::map<std::size_t, stage_t> m_stages;
    buffer_cache                   m_buffer_cache;
    std::vector<stage_t*>          m_stage_lu;
    bool                           m_connected = false;

  public:
    template<typename Map>
    terminal(Map& m)
    : m_map{m}
    , m_comm{std::make_unique<communicator>(oomph::detail::get_communicator(m.m_context))}
    , m_buffer_cache{m_comm.get()}
    {
    }

    terminal(terminal&&) noexcept = default;
    terminal& operator=(terminal&&) noexcept = default;

  public:
    void add_range(range_type const& view, int rank, int tag, std::size_t stage)
    {
        assert(!m_connected);
        //x==*,  y==1, z==1,  w==1 -> direct
        //x==Nx, y==*, z==1,  w==1 -> direct
        //x==Nx, y==Ny z==*,  w==1 -> direct
        //x==Nx, y==Ny z==Nz, w==* -> direct

        bool found_subset = (view.extents()[last_dim] != 1);
        bool direct = true;
        for (std::size_t d = 1; d < dim(); ++d)
        {
            auto const D = Layout::find(d);
            if (found_subset && view.extents()[D] != m_map.extents()[D])
            {
                direct = false;
                break;
            }
            else if (view.extents()[D] != 1)
            {
                found_subset = true;
            }
        }

        auto& s = m_stages[stage];

        if (direct)
        {
            auto ext = view.extents();
            ext[Layout::find(dim() - 1)] = m_map.line_size();
            auto const n_elements = product(ext);
            s.m_transport_ranges.push_back(transport_range{view,
                std::make_shared<message_buffer<T>>(
                    m_comm->make_buffer<T>(m_map.get_address(view.first()), n_elements)),
                rank, tag, true});
        }
        else
        {
            auto const n_elements = product(view.extents());
            auto const n_elements_slice = n_elements / view.extents()[last_dim];
            s.m_transport_ranges.push_back(transport_range{view,
                //m_comm->make_buffer<T>(n_elements),
                m_buffer_cache(n_elements, stage), rank, tag, false});
            T* ptr = s.m_transport_ranges.back().m_message->data();

            std::size_t const first_k = view.first()[last_dim];
            std::size_t const last_k = first_k + view.extents()[last_dim];
            for (std::size_t k = first_k; k < last_k; ++k)
            {
                auto first = view.first();
                first[last_dim] = k;
                auto ext = view.extents();
                ext[last_dim] = 1;

                s.m_serialization_ranges.push_back(
                    serialization_range{range_type{first, ext}, ptr});
                ptr += n_elements_slice;
            }

            std::sort(s.m_serialization_ranges.begin(), s.m_serialization_ranges.end(),
                [](auto const& a, auto const& b)
                {
                    auto const& first_a = a.m_range.first();
                    auto const& first_b = b.m_range.first();
                    for (std::size_t d = 0; d < dim(); ++d)
                    {
                        if (first_a[Layout::find(d)] < first_b[Layout::find(d)]) return true;
                        if (first_a[Layout::find(d)] > first_b[Layout::find(d)]) return false;
                    }
                    return true;
                });
        }
    }

  protected:
    void connect()
    {
        assert(!m_connected);
        assert(m_stages.size() > 0);
        m_stage_lu = std::vector<stage_t*>(m_stages.rbegin()->first + 1, nullptr);
        for (auto& kvp : m_stages) m_stage_lu[kvp.first] = &(kvp.second);
        m_connected = true;
    }

    stage_t* get_stage(std::size_t stage)
    {
        assert(stage < m_stage_lu.size());
        return m_stage_lu[stage];
    }

    T* serialize(serialization_range const& r, T* dst, vec coord,
        std::integral_constant<std::size_t, dim() - 1>)
    {
        static constexpr std::size_t D = Layout::find(dim() - 1);
        T const*                     src = m_map.get_address(coord);
        std::size_t const            n = r.m_range.extents()[D];
        for (std::size_t i = 0; i < n; ++i) dst[i] = src[i];

        return dst + n;
    }

    T const* serialize(serialization_range const& r, T const* src, vec coord,
        std::integral_constant<std::size_t, dim() - 1>)
    {
        static constexpr std::size_t D = Layout::find(dim() - 1);
        T*                           dst = m_map.get_address(coord);
        std::size_t const            n = r.m_range.extents()[D];
        for (std::size_t i = 0; i < n; ++i) dst[i] = src[i];
        return src + n;
    }

    template<std::size_t N, typename Ptr>
    Ptr serialize(serialization_range const& r, Ptr ptr, vec coord,
        std::integral_constant<std::size_t, N>)
    {
        static constexpr std::size_t D = Layout::find(N);
        std::size_t const            first = r.m_range.first()[D];
        std::size_t const            last = first + r.m_range.extents()[D];
        for (; coord[D] < last; ++coord[D])
            ptr = serialize(r, ptr, coord, std::integral_constant<std::size_t, N + 1>{});
        return ptr;
    }

    struct pack_handle
    {
        bool is_ready() const noexcept { return true; }
        void wait() {}
    };

    pack_handle pack(std::size_t stage)
    {
        assert(m_connected);
        stage_t* s = get_stage(stage);
        if (!s) return {};
        for (auto& r : s->m_serialization_ranges)
            serialize(r, r.m_ptr, r.m_range.first(), std::integral_constant<std::size_t, 1>());
        return {};
    }

    pack_handle unpack(std::size_t stage)
    {
        assert(m_connected);
        stage_t* s = get_stage(stage);
        if (!s) return {};
        for (auto& r : s->m_serialization_ranges)
            serialize(r, (T const*)r.m_ptr, r.m_range.first(),
                std::integral_constant<std::size_t, 1>());
        return {};
    }

    struct handle
    {
        communicator* m_comm;
        bool          is_ready() const noexcept { return m_comm->is_ready(); }
        void          progress() { m_comm->progress(); }
        void          wait() { m_comm->wait_all(); }
    };

    handle send(std::size_t stage)
    {
        assert(m_connected);
        stage_t* s = get_stage(stage);
        if (!s) return {m_comm.get()};
        for (auto& r : s->m_transport_ranges) m_comm->send(*r.m_message, r.m_rank, r.m_tag);
        return {m_comm.get()};
    }

    handle recv(std::size_t stage)
    {
        assert(m_connected);
        stage_t* s = get_stage(stage);
        if (!s) return {m_comm.get()};
        for (auto& r : s->m_transport_ranges) m_comm->recv(*r.m_message, r.m_rank, r.m_tag);
        return {m_comm.get()};
    }
};

} // namespace detail
} // namespace tensor
} // namespace oomph
