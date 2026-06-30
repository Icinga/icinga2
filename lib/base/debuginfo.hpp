// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DEBUGINFO_H
#define DEBUGINFO_H

#include "base/i2-base.hpp"
#include "base/string.hpp"

namespace icinga
{

/**
 * Debug information for a configuration element.
 *
 * @ingroup config
 */
struct DebugInfo
{
	String Path;

	int FirstLine{0};
	int FirstColumn{0};

	int LastLine{0};
	int LastColumn{0};
};

std::ostream& operator<<(std::ostream& out, const DebugInfo& val);

DebugInfo DebugInfoRange(const DebugInfo& start, const DebugInfo& end);

void ShowCodeLocation(std::ostream& out, const DebugInfo& di, bool verbose = true);

}

#endif /* DEBUGINFO_H */
