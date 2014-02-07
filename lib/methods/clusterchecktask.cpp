/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-present Icinga Development Team (http://www.icinga.org) *
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

#include "methods/clusterchecktask.h"
#include "cluster/endpoint.h"
#include "cluster/clusterlistener.h"
#include "icinga/cib.h"
#include "icinga/service.h"
#include "icinga/icingaapplication.h"
#include "base/application.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/scriptfunction.h"
#include "base/dynamictype.h"
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(ClusterCheck, &ClusterCheckTask::ScriptFunc);

CheckResult::Ptr ClusterCheckTask::ScriptFunc(const Service::Ptr&)
{
	double interval = Utility::GetTime() - Application::GetStartTime();

	if (interval > 60)
		interval = 60;

	double count_endpoints = 0;
	std::vector<String> not_connected_endpoints;
	std::vector<String> connected_endpoints;

	BOOST_FOREACH(const ClusterListener::Ptr& cluster_listener, DynamicType::GetObjects<ClusterListener>()) {
		String identity = cluster_listener->GetIdentity();

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
			count_endpoints++;

			if(!endpoint->IsConnected() && endpoint->GetName() != identity)
				not_connected_endpoints.push_back(endpoint->GetName());
			else if(endpoint->IsConnected() && endpoint->GetName() != identity)
				connected_endpoints.push_back(endpoint->GetName());
		}
	}

	std::sort(not_connected_endpoints.begin(), not_connected_endpoints.end());
	std::sort(connected_endpoints.begin(), connected_endpoints.end());

	ServiceState state = StateOK;
	String output = "Icinga 2 Cluster is running: Connected Endpoints: "+ Convert::ToString(connected_endpoints.size()) + " (" +
	    boost::algorithm::join(connected_endpoints, ",") + ").";

	if (not_connected_endpoints.size() > 0) {
		state = StateCritical;
		output = "Icinga 2 Cluster Problem: " + Convert::ToString(not_connected_endpoints.size()) +
		    " Endpoints (" + boost::algorithm::join(not_connected_endpoints, ",") + ") not connected.";
	}

	Dictionary::Ptr perfdata = make_shared<Dictionary>();
	perfdata->Set("num_endpoints", count_endpoints);
	perfdata->Set("num_conn_endpoints", connected_endpoints.size());
	perfdata->Set("num_not_conn_endpoints", not_connected_endpoints.size());

	CheckResult::Ptr cr = make_shared<CheckResult>();
	cr->SetOutput(output);
	cr->SetPerformanceData(perfdata);
	cr->SetState(state);
	cr->SetCheckSource(IcingaApplication::GetInstance()->GetNodeName());

	return cr;
}

