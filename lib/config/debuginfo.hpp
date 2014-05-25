/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef DEBUGINFO_H
#define DEBUGINFO_H

#include "config/i2-config.hpp"
#include "base/qstring.hpp"

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

	union
	{
		int FirstLine;
		int first_line;
	};

	union
	{
		int FirstColumn;
		int first_column;
	};

	union
	{
		int LastLine;
		int last_line;
	};

	union
	{
		int LastColumn;
		int last_column;
	};
};

I2_CONFIG_API std::ostream& operator<<(std::ostream& out, const DebugInfo& val);

I2_CONFIG_API DebugInfo DebugInfoRange(const DebugInfo& start, const DebugInfo& end);

I2_CONFIG_API void ShowCodeFragment(std::ostream& out, const DebugInfo& di, bool verbose);

}

#endif /* DEBUGINFO_H */
