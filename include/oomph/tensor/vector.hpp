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

#include <utility>

namespace oomph
{
namespace tensor
{
template<typename T, std::size_t N>
struct vector
{
    T m_data[N];

    static constexpr std::size_t size() noexcept { return N; }

    template<typename U>
    vector& operator=(vector<U, N> const& v)
    {
        for (std::size_t i = 0; i < N; ++i) m_data[i] = v[i];
    }

    constexpr T const* cbegin() const { return m_data; }
    constexpr T const* begin() const { return m_data; }
    constexpr T*       begin() { return m_data; }

    constexpr T const* cend() const { return cbegin() + N; }
    constexpr T const* end() const { return begin() + N; }
    constexpr T*       end() { return begin() + N; }

    constexpr const T* data() const noexcept { return m_data; }
    constexpr T*       data() noexcept { return m_data; }

    constexpr T operator[](std::size_t i) const noexcept { return m_data[i]; }
    T&          operator[](std::size_t i) noexcept { return m_data[i]; }

    template<typename U>
    vector& operator+=(vector<U, N> const& v) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) m_data[i] += v[i];
        return *this;
    }

    template<typename U>
    vector& operator+=(U const& u) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) m_data[i] += u;
        return *this;
    }

    template<typename U>
    vector& operator-=(vector<U, N> const& v) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) m_data[i] += v[i];
        return *this;
    }

    template<typename U>
    vector& operator-=(U const& u) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) m_data[i] += u;
        return *this;
    }

    template<typename U>
    vector& operator*=(vector<U, N> const& v) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) m_data[i] *= v[i];
        return *this;
    }

    template<typename U>
    vector& operator*=(U const& u) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) m_data[i] *= u;
        return *this;
    }

    template<typename U>
    vector& operator/=(vector<U, N> const& v) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) m_data[i] /= v[i];
        return *this;
    }

    template<typename U>
    vector& operator/=(U const& u) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) m_data[i] /= u;
        return *this;
    }
};

template<typename T, typename U, std::size_t N, std::size_t... I>
inline constexpr auto
add(vector<T, N> const& a, vector<U, N> const& b, std::index_sequence<I...>) noexcept
{
    using R = std::remove_reference_t<decltype(std::declval<T>() + std::declval<U>())>;
    return R{(a[I] + b[I])...};
}

template<typename T, typename U, std::size_t N>
inline constexpr auto
operator+(vector<T, N> const& a, vector<U, N> const& b) noexcept
{
    return add(a, b, std::make_index_sequence<N>{});
}

template<typename T, typename U, std::size_t N, std::size_t... I>
inline constexpr auto
subtract(vector<T, N> const& a, vector<U, N> const& b, std::index_sequence<I...>) noexcept
{
    using R = std::remove_reference_t<decltype(std::declval<T>() - std::declval<U>())>;
    return R{(a[I] - b[I])...};
}

template<typename T, typename U, std::size_t N>
inline constexpr auto
operator-(vector<T, N> const& a, vector<U, N> const& b) noexcept
{
    return subtract(a, b, std::make_index_sequence<N>{});
}

template<typename T, typename U, std::size_t N, std::size_t... I>
inline constexpr auto
multiply(vector<T, N> const& a, vector<U, N> const& b, std::index_sequence<I...>) noexcept
{
    using R = std::remove_reference_t<decltype(std::declval<T>() * std::declval<U>())>;
    return R{(a[I] * b[I])...};
}

template<typename T, typename U, std::size_t N>
inline constexpr auto
operator*(vector<T, N> const& a, vector<U, N> const& b) noexcept
{
    return multiply(a, b, std::make_index_sequence<N>{});
}

template<typename T, typename U, std::size_t N, std::size_t... I>
inline constexpr auto
divide(vector<T, N> const& a, vector<U, N> const& b, std::index_sequence<I...>) noexcept
{
    using R = std::remove_reference_t<decltype(std::declval<T>() / std::declval<U>())>;
    return R{(a[I] / b[I])...};
}

template<typename T, typename U, std::size_t N>
inline constexpr auto
operator/(vector<T, N> const& a, vector<U, N> const& b) noexcept
{
    return divide(a, b, std::make_index_sequence<N>{});
}

template<typename T, typename U, std::size_t N, std::size_t... I>
inline constexpr auto
dot(vector<T, N> const& a, vector<U, N> const& b, std::index_sequence<I...>) noexcept
{
    return (... + (a[I] * b[I]));
}

template<typename T, typename U, std::size_t N>
inline constexpr auto
dot(vector<T, N> const& a, vector<U, N> const& b) noexcept
{
    return dot(a, b, std::make_index_sequence<N>{});
}

template<typename T, std::size_t N, std::size_t... I>
inline constexpr auto
product(vector<T, N> const& v, std::index_sequence<I...>) noexcept
{
    return (... * v[I]);
}

template<typename T, std::size_t N>
inline constexpr auto
product(vector<T, N> const& v) noexcept
{
    return product(v, std::make_index_sequence<N>{});
}

template<std::size_t N, typename T, std::size_t... I>
inline constexpr vector<T, N>
make_uniform(T const& value, std::index_sequence<I...>) noexcept
{
    T const* const vref = &value;
    return {vref[I - I]...};
}

template<std::size_t N, typename T>
inline constexpr vector<T, N>
make_uniform(T const& value) noexcept
{
    return make_uniform<N>(value, std::make_index_sequence<N>{});
}

} // namespace tensor
} // namespace oomph
