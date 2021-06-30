#pragma once

#include <utility>

namespace oomph
{
namespace util
{
struct moved_bit
{
    bool m_moved = false;

    moved_bit() = default;
    moved_bit(bool state) noexcept
    : m_moved{state}
    {
    }
    moved_bit(const moved_bit&) = default;
    moved_bit(moved_bit&& other) noexcept
    : m_moved{std::exchange(other.m_moved, true)}
    {
    }

    moved_bit& operator=(const moved_bit&) = default;
    moved_bit& operator=(moved_bit&& other) noexcept
    {
        m_moved = std::exchange(other.m_moved, true);
        return *this;
    }

    operator bool() const { return m_moved; }
};

} // namespace util
} // namespace oomph
