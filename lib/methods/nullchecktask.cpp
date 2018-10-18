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
#include "methods/nullchecktask.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, NullCheck, &NullCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void NullCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	if (resolvedMacros && !useResolvedMacros)
		return;

	String output = "Hello from ";
	output += IcingaApplication::GetInstance()->GetNodeName();

	cr->SetOutput(output);
	cr->SetPerformanceData(new Array({
		new PerfdataValue("time", Convert::ToDouble(Utility::GetTime()))
	}));
	cr->SetState(ServiceOK);

	checkable->ProcessCheckResult(cr);
}
