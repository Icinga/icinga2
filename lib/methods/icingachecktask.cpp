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

#include "methods/icingachecktask.h"
#include "icinga/cib.h"
#include "icinga/service.h"
#include "icinga/icingaapplication.h"
#include "base/application.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/scriptfunction.h"
#include "base/dynamictype.h"

using namespace icinga;

REGISTER_SCRIPTFUNCTION(IcingaCheck, &IcingaCheckTask::ScriptFunc);

CheckResult::Ptr IcingaCheckTask::ScriptFunc(const Service::Ptr&)
{
	double interval = Utility::GetTime() - Application::GetStartTime();

	if (interval > 60)
		interval = 60;

	Dictionary::Ptr perfdata = make_shared<Dictionary>();
	perfdata->Set("active_checks", CIB::GetActiveChecksStatistics(interval) / interval);
	perfdata->Set("passive_checks", CIB::GetPassiveChecksStatistics(interval) / interval);

	double min_latency = -1, max_latency = 0, sum_latency = 0;
	int count_latency = 0;
	double min_execution_time = -1, max_execution_time = 0, sum_execution_time = 0;
	int count_execution_time = 0;

	double services_ok, services_warn, services_crit, services_unknown, services_pending, services_unreachable,
	    services_flapping, services_in_downtime, services_acknowledged = 0;
	double hosts_up, hosts_down, hosts_unreachable, hosts_pending, hosts_flapping, hosts_in_downtime, hosts_acknowledged = 0;

	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		ObjectLock olock(service);

		CheckResult::Ptr cr = service->GetLastCheckResult();

		/* latency */
		double latency = Service::CalculateLatency(cr);

		if (min_latency == -1 || latency < min_latency)
			min_latency = latency;

		if (latency > max_latency)
			max_latency = latency;

		sum_latency += latency;
		count_latency++;

		/* execution_time */
		double execution_time = Service::CalculateExecutionTime(cr);

		if (min_execution_time == -1 || execution_time < min_execution_time)
			min_execution_time = execution_time;

		if (execution_time > max_execution_time)
			max_execution_time = execution_time;

		sum_execution_time += execution_time;
		count_execution_time++;

		/* states */
		if (service->GetState() == StateOK)
			services_ok++;
		if (service->GetState() == StateWarning)
			services_warn++;
		if (service->GetState() == StateCritical)
			services_crit++;
		if (service->GetState() == StateUnknown)
			services_unknown++;

		/* pending, unreachable */
		if (!cr)
			services_pending++;
		if (!service->IsReachable())
			services_unreachable++;

		/* flapping, downtime, acknowledgements */
		if (service->IsFlapping())
			services_flapping++;
		if (service->IsInDowntime())
			services_in_downtime++;
		if (service->IsAcknowledged())
			services_acknowledged++;
	}

	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
		ObjectLock olock(host);

		if (host->GetState() == HostUp)
			hosts_up++;
		if (host->GetState() == HostDown)
			hosts_down++;
		if (host->GetState() == HostUnreachable)
			hosts_unreachable++;

		Service::Ptr hc = host->GetCheckService();

		if (!hc) {
			hosts_pending++;
			continue; /* skip host service check counting */
		}

		if (!hc->GetLastCheckResult())
			hosts_pending++;

		if (hc->IsFlapping())
			hosts_flapping++;
		if (hc->IsInDowntime())
			hosts_in_downtime++;
		if (hc->IsAcknowledged())
			hosts_acknowledged++;
	}

	perfdata->Set("min_latency", min_latency);
	perfdata->Set("max_latency", max_latency);
	perfdata->Set("avg_latency", sum_latency / count_latency);
	perfdata->Set("min_execution_time", min_latency);
	perfdata->Set("max_execution_time", max_latency);
	perfdata->Set("avg_execution_time", sum_execution_time / count_execution_time);

	perfdata->Set("num_services_ok", services_ok);
	perfdata->Set("num_services_warn", services_warn);
	perfdata->Set("num_services_crit", services_crit);
	perfdata->Set("num_services_unknown", services_unknown);
	perfdata->Set("num_services_pending", services_pending);
	perfdata->Set("num_services_unreachable", services_unreachable);
	perfdata->Set("num_services_flapping", services_flapping);
	perfdata->Set("num_services_in_downtime", services_in_downtime);
	perfdata->Set("num_services_acknowledged", services_acknowledged);

	perfdata->Set("num_hosts_up", hosts_up);
	perfdata->Set("num_hosts_down", hosts_down);
	perfdata->Set("num_hosts_unreachable", hosts_unreachable);
	perfdata->Set("num_hosts_flapping", hosts_flapping);
	perfdata->Set("num_hosts_in_downtime", hosts_in_downtime);
	perfdata->Set("num_hosts_acknowledged", hosts_acknowledged);

	CheckResult::Ptr cr = make_shared<CheckResult>();
	cr->SetOutput("Icinga 2 is running.");
	cr->SetPerformanceData(perfdata);
	cr->SetState(StateOK);
	cr->SetCheckSource(IcingaApplication::GetInstance()->GetNodeName());

	return cr;
}

