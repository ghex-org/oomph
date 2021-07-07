#pragma once

#include <oomph/request.hpp>

namespace oomph
{
class request::impl
{
  public:
    ~impl();

    bool is_ready() { return true; }

    void wait() {}
};

template<>
request::request<>();

} // namespace oomph
