/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

REGISTER_SCRIPTFUNCTION_NS(Internal, ClusterCheck, &ClusterCheckTask::ScriptFunc);

void ClusterCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
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

	String connected_endpoints = FormatArray(status->Get("conn_endpoints"));
	String not_connected_endpoints = FormatArray(status->Get("not_conn_endpoints"));

	if (status->Get("num_not_conn_endpoints") > 0) {
		cr->SetState(ServiceCritical);
		cr->SetOutput("Icinga 2 Cluster Problem: " + Convert::ToString(status->Get("num_not_conn_endpoints")) +
		    " Endpoints (" + not_connected_endpoints + ") not connected.");
	} else {
		cr->SetState(ServiceOK);
		cr->SetOutput("Icinga 2 Cluster is running: Connected Endpoints: "+ Convert::ToString(status->Get("num_conn_endpoints")) +
		    " (" + connected_endpoints + ").");
	}

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
