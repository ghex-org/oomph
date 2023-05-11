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

#include <hwmalloc/numa.hpp>
#include <oomph/config.hpp>
#include <oomph/communicator.hpp>
#include <oomph/context.hpp>
#include <oomph/util/heap_pimpl.hpp>

namespace oomph
{

// singleton
class communicator_set
{
  private:
    struct impl;
    util::heap_pimpl<impl> m_impl;

  private:
    communicator_set();
    communicator_set(communicator_set const&) = delete;
    communicator_set& operator=(communicator_set const&) = delete;

  public:
    ~communicator_set() = default;

  public:
    static communicator_set& get();

  public:
    void insert(context_impl const* ctxt, communicator_impl* comm);

    void erase(context_impl const* ctxt, communicator_impl* comm);

    void erase(context_impl const* ctxt);

    void progress(context_impl const* ctxt);
};

} // namespace oomph
