/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/util/pimpl.hpp>

namespace oomph
{
class request
{
  public:
    class impl;

  private:
    using pimpl = util::pimpl<impl, 16, 8>;

  public:
    pimpl m_impl;

    request();

    request(request::impl&& r);

    template<typename... Args>
    request(Args... args);

    request(request&&);

    request& operator=(request&&);

    ~request();

  private:
  public:
    bool is_ready();
    void wait();
    bool cancel();
};

} // namespace oomph
