/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef PLUGINUTILITY_H
#define PLUGINUTILITY_H

#include "icinga/i2-icinga.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include "base/dictionary.h"
#include "base/dynamicobject.h"
#include <vector>

namespace icinga
{

/**
 * Utility functions for plugin-based checks.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API PluginUtility
{
public:
	static ServiceState ExitStatusToState(int exitStatus);
	static CheckResult::Ptr ParseCheckOutput(const String& output);

	static Value ParsePerfdata(const String& perfdata);
	static String FormatPerfdata(const Value& perfdata);

private:
	PluginUtility(void);
};

}

#endif /* PLUGINUTILITY_H */
