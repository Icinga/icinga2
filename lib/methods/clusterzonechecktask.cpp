/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "methods/clusterzonechecktask.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/perfdatavalue.hpp"
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "remote/zone.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(ClusterZoneCheck, &ClusterZoneCheckTask::ScriptFunc);

void ClusterZoneCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		cr->SetOutput("No API listener is configured for this instance.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();
	Value raw_command = commandObj->GetCommandLine();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.push_back(std::make_pair("service", service));
	resolvers.push_back(std::make_pair("host", host));
	resolvers.push_back(std::make_pair("command", commandObj));
	resolvers.push_back(std::make_pair("icinga", IcingaApplication::GetInstance()));

	String zoneName = MacroProcessor::ResolveMacros("$cluster_zone$", resolvers, checkable->GetLastCheckResult(),
	    NULL, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (zoneName.IsEmpty()) {
		cr->SetOutput("Macro 'cluster_zone' must be set.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	Zone::Ptr zone = Zone::GetByName(zoneName);

	if (!zone) {
		cr->SetOutput("Zone '" + zoneName + "' does not exist.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	bool connected = false;
	double zoneLag = 0;

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, zone->GetEndpoints()) {
		if (endpoint->GetConnected())
			connected = true;

		double eplag = ApiListener::CalculateZoneLag(endpoint);

		if (eplag > 0 && eplag > zoneLag)
			zoneLag = eplag;
	}

	if (!connected) {
		cr->SetState(ServiceCritical);
		cr->SetOutput("Zone '" + zoneName + "' is not connected. Log lag: " + Utility::FormatDuration(zoneLag));
	} else {
		cr->SetState(ServiceOK);
		cr->SetOutput("Zone '" + zoneName + "' is connected. Log lag: " + Utility::FormatDuration(zoneLag));
	}

	Array::Ptr perfdata = new Array();
	perfdata->Add(new PerfdataValue("slave_lag", zoneLag));
	cr->SetPerformanceData(perfdata);

	checkable->ProcessCheckResult(cr);
}
