/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <iostream>
#include "context_bind.hpp"

int main()
{
    std::cout << "#ifndef OOMPH_SIZES_H_INCLUDED\n";
    std::cout << "#define OOMPH_SIZES_H_INCLUDED\n";    
    std::cout << "\n";    
    size_t rsize = sizeof(oomph::send_request)>sizeof(oomph::recv_request)?sizeof(oomph::send_request):sizeof(oomph::recv_request);
    std::cout << "#define OOMPH_REQUEST_SIZE " << rsize << "\n";
    std::cout << "\n";
    std::cout << "#endif /* OOMPH_SIZES_H_INCLUDED */\n";
}
