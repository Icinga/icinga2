/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "base/exception.hpp"

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

	int FirstLine;
	int FirstColumn;

	int LastLine;
	int LastColumn;
};

I2_BASE_API std::ostream& operator<<(std::ostream& out, const DebugInfo& val);

I2_BASE_API DebugInfo DebugInfoRange(const DebugInfo& start, const DebugInfo& end);

I2_BASE_API void ShowCodeFragment(std::ostream& out, const DebugInfo& di, bool verbose);

struct errinfo_debuginfo_;
typedef boost::error_info<struct errinfo_debuginfo_, DebugInfo> errinfo_debuginfo;

I2_BASE_API std::string to_string(const errinfo_debuginfo& e);

}

#endif /* DEBUGINFO_H */
