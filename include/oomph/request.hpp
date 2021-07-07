#pragma once

#include <oomph/util/pimpl.hpp>

namespace oomph
{
class request
{
  public:
  private:
    class impl;
    using pimpl = util::pimpl<impl, 8, 8>;

  public:
    pimpl m_impl;

    template<typename... Args>
    request(Args... args);

    request(request&&);

    ~request();

  private:
  public:
    bool is_ready();
    void wait();
};

} // namespace oomph
