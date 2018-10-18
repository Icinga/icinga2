/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/randomchecktask.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, RandomCheck, &RandomCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void RandomCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	if (resolvedMacros && !useResolvedMacros)
		return;

	double now = Utility::GetTime();
	double uptime = now - Application::GetStartTime();

	String output = "Hello from " + IcingaApplication::GetInstance()->GetNodeName()
		+ ". Icinga 2 has been running for " + Utility::FormatDuration(uptime)
		+ ". Version: " + Application::GetAppVersion();

	cr->SetOutput(output);

	double random = Utility::Random() % 1000;

	cr->SetPerformanceData(new Array({
		new PerfdataValue("time", now),
		new PerfdataValue("value", random),
		new PerfdataValue("value_1m", random * 0.9),
		new PerfdataValue("value_5m", random * 0.8),
		new PerfdataValue("uptime", uptime),
	}));

	cr->SetState(static_cast<ServiceState>(Utility::Random() % 4));

	checkable->ProcessCheckResult(cr);
}
