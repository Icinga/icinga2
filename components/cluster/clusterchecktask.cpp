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

#include "cluster/clusterchecktask.h"
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
	Dictionary::Ptr status;
	BOOST_FOREACH(const ClusterListener::Ptr& cluster_listener, DynamicType::GetObjects<ClusterListener>()) {
		/* XXX there's only one cluster listener */
		status = cluster_listener->GetClusterStatus();
	}

	String connected_endpoints = FormatArray(status->Get("conn_endpoints"));
	String not_connected_endpoints = FormatArray(status->Get("not_conn_endpoints"));

	/* remove unneeded perfdata */
	status->Set("conn_endpoints", Empty);
	status->Set("not_conn_endpoints", Empty);

	ServiceState state = StateOK;
	String output = "Icinga 2 Cluster is running: Connected Endpoints: "+ Convert::ToString(status->Get("num_conn_endpoints")) + " (" +
	    connected_endpoints + ").";

	if (status->Get("num_not_conn_endpoints") > 0) {
		state = StateCritical;
		output = "Icinga 2 Cluster Problem: " + Convert::ToString(status->Get("num_not_conn_endpoints")) +
		    " Endpoints (" + not_connected_endpoints + ") not connected.";
	}

	CheckResult::Ptr cr = make_shared<CheckResult>();
	cr->SetOutput(output);
	cr->SetPerformanceData(status);
	cr->SetState(state);
	cr->SetCheckSource(IcingaApplication::GetInstance()->GetNodeName());

	return cr;
}

String ClusterCheckTask::FormatArray(const Array::Ptr& arr)
{
	bool first = true;
	String str;

	if (arr) {
		ObjectLock olock(arr);
		BOOST_FOREACH(const Value& value, arr) {
			if (first)
				first = false;
			else
				str += ",";

			str += Convert::ToString(value);
		}
	}

	return str;
}

