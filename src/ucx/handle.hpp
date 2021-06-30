#pragma once

#include "./error.hpp"

namespace oomph
{
struct handle
{
    void*       m_ptr;
    std::size_t m_size;
};

} // namespace oomph
