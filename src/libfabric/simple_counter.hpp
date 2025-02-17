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

#include "oomph_libfabric_defines.hpp"
//
#include <atomic>
#include <type_traits>
#include <iostream>

#ifdef OOMPH_LIBFABRIC_HAVE_PERFORMANCE_COUNTERS
#define PERFORMANCE_COUNTER_ENABLED true
#else
#define PERFORMANCE_COUNTER_ENABLED false
#endif

//
// This class is intended to provide a simple atomic counter that can be used as a
// performance counter, but that can be disabled at compile time so that it
// has no performance cost when not used. It is only to avoid a lot of #ifdef
// statements in user code that we collect everything in here and then provide
// the performance counter that will simply do nothing when disabled - but
// still allow code that uses the counters in arithmetic to compile.
//
namespace oomph
{
namespace libfabric
{
template<typename T, bool enabled = PERFORMANCE_COUNTER_ENABLED,
    typename Enable = std::enable_if_t<std::is_integral<T>::value>>
struct simple_counter
{
};

// --------------------------------------------------------------------
// specialization for performance counters Enabled
// we provide an atomic<T> that can be incremented or added/subtracted to
template<typename T>
struct simple_counter<T, true>
{
    simple_counter()
    : value_{T()}
    {
    }

    simple_counter(const T& init)
    : value_{init}
    {
    }

    inline operator T() const { return value_; }

    inline T operator=(const T& x) { return value_ = x; }

    inline T operator++() { return ++value_; }

    inline T operator++(int x) { return (value_ += x); }

    inline T operator+=(const T& rhs) { return (value_ += rhs); }

    inline T operator--() { return --value_; }

    inline T operator--(int x) { return (value_ -= x); }

    inline T operator-=(const T& rhs) { return (value_ -= rhs); }

    friend std::ostream& operator<<(std::ostream& os, const simple_counter<T, true>& x)
    {
        os << x.value_;
        return os;
    }

    std::atomic<T> value_;
};

// --------------------------------------------------------------------
// specialization for performance counters Disabled
// just return dummy values so that arithmetic operations compile ok
template<typename T>
struct simple_counter<T, false>
{
    simple_counter() {}

    simple_counter(const T&) {}

    inline operator T() const { return 0; }

    //        inline bool operator==(const T&) { return true; }

    inline T operator=(const T&) { return 0; }

    inline T operator++() { return 0; }

    inline T operator++(int) { return 0; }

    inline T operator+=(const T&) { return 0; }

    inline T operator--() { return 0; }

    inline T operator--(int) { return 0; }

    inline T operator-=(const T&) { return 0; }

    friend std::ostream& operator<<(std::ostream& os, const simple_counter<T, false>&)
    {
        os << "undefined";
        return os;
    }
};
} // namespace libfabric
} // namespace oomph
