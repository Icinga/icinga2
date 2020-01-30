/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
