#pragma once

#include <oomph/communicator.hpp>
#include "./context.hpp"

namespace oomph
{
class communicator::impl
{
  public:
    context_impl* m_context;
};

} // namespace oomph
