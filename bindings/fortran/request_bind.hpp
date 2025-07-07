/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef INCLUDED_OOMPH_FORTRAN_REQUEST_BIND_HPP
#define INCLUDED_OOMPH_FORTRAN_REQUEST_BIND_HPP

#include <bindings/fortran/oomph_sizes.hpp>
#include <cstdint>
#include <oomph/request.hpp>

namespace oomph { namespace fort {
    struct frequest_type
    {
        int8_t data[OOMPH_REQUEST_SIZE];
        bool recv_request;
    };
}}    // namespace oomph::fort

#endif /* INCLUDED_OOMPH_FORTRAN_REQUEST_BIND_HPP */
