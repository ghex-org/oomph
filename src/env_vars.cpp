/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */

extern char** environ;

#include <iostream>
#include <algorithm>
#include "./env_vars.hpp"

namespace oomph
{

env_vars::env_vars()
{
    char** s = environ;
    for (; *s; s++)
    {
        std::string_view v(*s);
        auto const       pos = v.find('=');
        if (pos == std::string_view::npos) m_vars[v] = std::string_view();
        else
            m_vars[v.substr(0, pos)] = v.substr(pos + 1);
    }
}

void
env_vars::print() const noexcept
{
    for (auto const& p : m_vars) std::cout << p.first << "=" << p.second << "\n";
    std::cout << std::endl;
}

std::vector<std::pair<std::string_view, std::string_view>>
env_vars::key_contains(char const* k) const
{
    std::vector<std::pair<std::string_view, std::string_view>> res;

    std::string_view v(k);
    auto pred = [v](auto const& p) { return p.first.find(v) != std::string_view::npos; };
    auto it = m_vars.begin();
    while (true)
    {
        it = std::find_if(it, m_vars.end(), pred);
        if (it == m_vars.end()) break;
        else
        {
            res.push_back(*it);
            ++it;
            if (it == m_vars.end()) break;
        }
    }
    return res;
}

std::optional<std::string_view>
env_vars::find(char const* k) const noexcept
{
    auto it = m_vars.find(std::string_view(k));
    if (it == m_vars.end()) return {};
    else
        return it->second;
}

} // namespace oomph
