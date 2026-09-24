// SPDX-FileCopyrightText: 2019 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPLY_UTILITY_H
#define APPLY_UTILITY_H

#include "base/debuginfo.hpp"
#include "base/string.hpp"
#include "config/expression.hpp"
#include <memory>
#include <vector>

namespace icinga
{

namespace ApplyUtility
{

std::unique_ptr<Expression> MakeCommonZone(String parent_object_zone, String apply_rule_zone, const DebugInfo& debug_info);

}

}

#endif /* APPLY_UTILITY_H */
