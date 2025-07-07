/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */

// paths relative to backend
#include <communicator.hpp>
#include <context.hpp>

namespace oomph {
    communicator_impl* context_impl::get_communicator()
    {
        auto comm = new communicator_impl{this};
        m_comms_set.insert(comm);
        return comm;
    }

    char const* context_impl::get_transport_option(std::string const& opt)
    {
        if (opt == "name") { return "mpi"; }
        else { return "unspecified"; }
    }

}    // namespace oomph
