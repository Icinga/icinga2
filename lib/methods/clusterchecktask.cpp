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

#include "methods/clusterchecktask.hpp"
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "icinga/cib.hpp"
#include "icinga/service.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/function.hpp"
#include "base/configtype.hpp"
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, ClusterCheck, &ClusterCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void ClusterCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	if (resolvedMacros && !useResolvedMacros)
		return;

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		cr->SetOutput("No API listener is configured for this instance.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	std::pair<Dictionary::Ptr, Dictionary::Ptr> stats = listener->GetStatus();

	Dictionary::Ptr status = stats.first;

	/* use feature stats perfdata */
	std::pair<Dictionary::Ptr, Array::Ptr> feature_stats = CIB::GetFeatureStats();
	cr->SetPerformanceData(feature_stats.second);

	int numConnEndpoints = status->Get("num_conn_endpoints");
	int numNotConnEndpoints = status->Get("num_not_conn_endpoints");

	String output = "Icinga 2 Cluster";

	if (numNotConnEndpoints > 0) {
		output += " Problem: " + Convert::ToString(numNotConnEndpoints) + " endpoints are not connected.";
		output += "\n(" + FormatArray(status->Get("not_conn_endpoints")) + ")";

		cr->SetState(ServiceCritical);
	} else {
		output += " OK: " + Convert::ToString(numConnEndpoints) + " endpoints are connected.";
		output += "\n(" + FormatArray(status->Get("conn_endpoints")) + ")";

		cr->SetState(ServiceOK);
	}

	cr->SetOutput(output);

	checkable->ProcessCheckResult(cr);
}

String ClusterCheckTask::FormatArray(const Array::Ptr& arr)
{
	bool first = true;
	String str;

	if (arr) {
		ObjectLock olock(arr);
		for (const Value& value : arr) {
			if (first)
				first = false;
			else
				str += ", ";

			str += Convert::ToString(value);
		}
	}

	return str;
}
