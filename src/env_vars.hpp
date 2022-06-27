/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <map>
#include <string_view>
#include <optional>
#include <vector>

namespace oomph
{

class env_vars
{
  private:
    using map_type = std::map<std::string_view, std::string_view>;

    map_type m_vars;

  public:
    env_vars();

    void print() const noexcept;

    std::vector<std::pair<std::string_view, std::string_view>> key_contains(char const* k) const;

    std::optional<std::string_view> find(char const* k) const noexcept;
};

} // namespace oomph
