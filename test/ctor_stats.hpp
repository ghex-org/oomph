/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/util/moved_bit.hpp>
#include <iostream>
#include <map>
#include <string>

// helper classes and functions for keeping track of lifetime
struct ctor_stats_data
{
    int n_ctor = 0;

    int n_dtor = 0;
    int n_dtor_of_moved = 0;

    int n_move_ctor = 0;
    int n_move_ctor_of_moved = 0;

    int n_move_assign = 0;
    int n_move_assign_to_moved = 0;
    int n_move_assign_of_moved = 0;
    int n_move_assign_of_moved_to_moved = 0;

    int n_calls = 0;

    int alloc_ref_count = 0;

    friend std::ostream& operator<<(std::ostream& os, ctor_stats_data const& d)
    {
        os << "stats:"
           << "\n  n_ctor = " << d.n_ctor << "\n"
           << "\n  n_dtor = " << d.n_ctor << "\n  n_dtor_of_moved = " << d.n_dtor_of_moved << "\n"
           << "\n  n_move_ctor = " << d.n_move_ctor
           << "\n  n_move_ctor_of_moved = " << d.n_move_ctor_of_moved << "\n"
           << "\n  n_move_assign = " << d.n_move_assign
           << "\n  n_move_assign_to_moved = " << d.n_move_assign_to_moved
           << "\n  n_move_assign_of_moved = " << d.n_move_assign_of_moved
           << "\n  n_move_assign_of_moved_to_moved = " << d.n_move_assign_of_moved_to_moved << "\n"
           << "\n  n_calls = " << d.n_calls << "\n"
           << "\n  alloc_ref_count = " << d.alloc_ref_count << "\n";
        return os;
    }
};

struct ctor_stats
{
    ctor_stats_data*       data;
    oomph::util::moved_bit moved;

    ctor_stats(ctor_stats_data& d)
    : data{&d}
    {
        ++(data->n_ctor);
        ++(data->alloc_ref_count);
    }

    ~ctor_stats()
    {
        if (!moved)
        {
            ++(data->n_dtor);
            --(data->alloc_ref_count);
        }
        else
            ++(data->n_dtor_of_moved);
    }

    ctor_stats(ctor_stats const&) = delete;
    ctor_stats& operator=(ctor_stats const&) = delete;

    ctor_stats(ctor_stats&& other)
    : data{other.data}
    , moved{std::move(other.moved)}
    {
        if (!moved) ++(data->n_move_ctor);
        else
            ++(data->n_move_ctor_of_moved);
    }

    ctor_stats& operator=(ctor_stats&& other)
    {
        data = other.data;
        if (!moved)
        {
            if (!other.moved) ++(data->n_move_assign);
            else
                ++(data->n_move_assign_of_moved);
        }
        else
        {
            if (!other.moved) ++(data->n_move_assign_to_moved);
            else
                ++(data->n_move_assign_of_moved_to_moved);
        }
        moved = std::move(other.moved);
        return *this;
    }

    operator bool() const noexcept { return !moved; }

    void call() { ++(data->n_calls); }
};

// registry for storing and retrieving stats
struct function_registry
{
    std::map<std::string, ctor_stats_data> m_data;

    template<typename F>
    F make(std::string const& id)
    {
        return F(m_data[id]);
    }

    ctor_stats_data const& operator[](std::string const& id) { return m_data[id]; }
};
