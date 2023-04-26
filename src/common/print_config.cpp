/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <iostream>

namespace oomph
{
void
print_config()
{
    std::cout << std::endl;
    std::cout << " -- OOMPH compile configuration:" << std::endl;
    std::cout << std::endl;
#include <oomph/cmake_config.inc>
    std::cout << std::endl;
}

} // namespace oomph