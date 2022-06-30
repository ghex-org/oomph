/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <gtest/gtest.h>
#include "../src/env_vars.hpp"

TEST(env_vars, print)
{
    using namespace oomph;

    env_vars ev;
    ev.print();

    std::cout << "==========" << std::endl;
    for (const auto& p : ev.key_contains("HOM"))
    {
        std::cout << p.first << " = " << p.second << std::endl;
    }
    std::cout << "==========" << std::endl;
    for (const auto& p : ev.key_contains("hom"))
    {
        std::cout << p.first << " = " << p.second << std::endl;
    }
    std::cout << "==========" << std::endl;
    for (const auto& p : ev.key_contains("H"))
    {
        std::cout << p.first << " = " << p.second << std::endl;
    }

    std::cout << "==========" << std::endl;
    if (auto v = ev.find("HOME"))
    {
        std::cout << *v << std::endl;
    }
}


