/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "icinga/cib.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"

using namespace icinga;

RingBuffer CIB::m_ActiveHostChecksStatistics(15 * 60);
RingBuffer CIB::m_ActiveServiceChecksStatistics(15 * 60);
RingBuffer CIB::m_PassiveHostChecksStatistics(15 * 60);
RingBuffer CIB::m_PassiveServiceChecksStatistics(15 * 60);

void CIB::UpdateActiveHostChecksStatistics(long tv, int num)
{
	m_ActiveHostChecksStatistics.InsertValue(tv, num);
}

void CIB::UpdateActiveServiceChecksStatistics(long tv, int num)
{
	m_ActiveServiceChecksStatistics.InsertValue(tv, num);
}

int CIB::GetActiveHostChecksStatistics(long timespan)
{
	return m_ActiveHostChecksStatistics.UpdateAndGetValues(Utility::GetTime(), timespan);
}

int CIB::GetActiveServiceChecksStatistics(long timespan)
{
	return m_ActiveServiceChecksStatistics.UpdateAndGetValues(Utility::GetTime(), timespan);
}

void CIB::UpdatePassiveHostChecksStatistics(long tv, int num)
{
	m_PassiveServiceChecksStatistics.InsertValue(tv, num);
}

void CIB::UpdatePassiveServiceChecksStatistics(long tv, int num)
{
	m_PassiveServiceChecksStatistics.InsertValue(tv, num);
}

int CIB::GetPassiveHostChecksStatistics(long timespan)
{
	return m_PassiveHostChecksStatistics.UpdateAndGetValues(Utility::GetTime(), timespan);
}

int CIB::GetPassiveServiceChecksStatistics(long timespan)
{
	return m_PassiveServiceChecksStatistics.UpdateAndGetValues(Utility::GetTime(), timespan);
}

CheckableCheckStatistics CIB::CalculateHostCheckStats()
{
	double min_latency = -1, max_latency = 0, sum_latency = 0;
	int count_latency = 0;
	double min_execution_time = -1, max_execution_time = 0, sum_execution_time = 0;
	int count_execution_time = 0;
	bool checkresult = false;

	for (const Host::Ptr& host : ConfigType::GetObjectsByType<Host>()) {
		ObjectLock olock(host);

		CheckResult::Ptr cr = host->GetLastCheckResult();

		if (!cr)
			continue;

		/* set to true, we have a checkresult */
		checkresult = true;

		/* latency */
		double latency = cr->CalculateLatency();

		if (min_latency == -1 || latency < min_latency)
			min_latency = latency;

		if (latency > max_latency)
			max_latency = latency;

		sum_latency += latency;
		count_latency++;

		/* execution_time */
		double execution_time = cr->CalculateExecutionTime();

		if (min_execution_time == -1 || execution_time < min_execution_time)
			min_execution_time = execution_time;

		if (execution_time > max_execution_time)
			max_execution_time = execution_time;

		sum_execution_time += execution_time;
		count_execution_time++;
	}

	if (!checkresult) {
		min_latency = 0;
		min_execution_time = 0;
	}

	CheckableCheckStatistics ccs;

	ccs.min_latency = min_latency;
	ccs.max_latency = max_latency;
	ccs.avg_latency = sum_latency / count_latency;
	ccs.min_execution_time = min_execution_time;
	ccs.max_execution_time = max_execution_time;
	ccs.avg_execution_time = sum_execution_time / count_execution_time;

	return ccs;
}

CheckableCheckStatistics CIB::CalculateServiceCheckStats()
{
	double min_latency = -1, max_latency = 0, sum_latency = 0;
	int count_latency = 0;
	double min_execution_time = -1, max_execution_time = 0, sum_execution_time = 0;
	int count_execution_time = 0;
	bool checkresult = false;

	for (const Service::Ptr& service : ConfigType::GetObjectsByType<Service>()) {
		ObjectLock olock(service);

		CheckResult::Ptr cr = service->GetLastCheckResult();

		if (!cr)
			continue;

		/* set to true, we have a checkresult */
		checkresult = true;

		/* latency */
		double latency = cr->CalculateLatency();

		if (min_latency == -1 || latency < min_latency)
			min_latency = latency;

		if (latency > max_latency)
			max_latency = latency;

		sum_latency += latency;
		count_latency++;

		/* execution_time */
		double execution_time = cr->CalculateExecutionTime();

		if (min_execution_time == -1 || execution_time < min_execution_time)
			min_execution_time = execution_time;

		if (execution_time > max_execution_time)
			max_execution_time = execution_time;

		sum_execution_time += execution_time;
		count_execution_time++;
	}

	if (!checkresult) {
		min_latency = 0;
		min_execution_time = 0;
	}

	CheckableCheckStatistics ccs;

	ccs.min_latency = min_latency;
	ccs.max_latency = max_latency;
	ccs.avg_latency = sum_latency / count_latency;
	ccs.min_execution_time = min_execution_time;
	ccs.max_execution_time = max_execution_time;
	ccs.avg_execution_time = sum_execution_time / count_execution_time;

	return ccs;
}

ServiceStatistics CIB::CalculateServiceStats()
{
	ServiceStatistics ss = {};

	for (const Service::Ptr& service : ConfigType::GetObjectsByType<Service>()) {
		ObjectLock olock(service);

		CheckResult::Ptr cr = service->GetLastCheckResult();

		if (service->GetState() == ServiceOK)
			ss.services_ok++;
		if (service->GetState() == ServiceWarning)
			ss.services_warning++;
		if (service->GetState() == ServiceCritical)
			ss.services_critical++;
		if (service->GetState() == ServiceUnknown)
			ss.services_unknown++;

		if (!cr)
			ss.services_pending++;
		if (!service->IsReachable())
			ss.services_unreachable++;

		if (service->IsFlapping())
			ss.services_flapping++;
		if (service->IsInDowntime())
			ss.services_in_downtime++;
		if (service->IsAcknowledged())
			ss.services_acknowledged++;
	}

	return ss;
}

HostStatistics CIB::CalculateHostStats()
{
	HostStatistics hs = {};

	for (const Host::Ptr& host : ConfigType::GetObjectsByType<Host>()) {
		ObjectLock olock(host);

		if (host->IsReachable()) {
			if (host->GetState() == HostUp)
				hs.hosts_up++;
			if (host->GetState() == HostDown)
				hs.hosts_down++;
		} else
			hs.hosts_unreachable++;

		if (!host->GetLastCheckResult())
			hs.hosts_pending++;

		if (host->IsFlapping())
			hs.hosts_flapping++;
		if (host->IsInDowntime())
			hs.hosts_in_downtime++;
		if (host->IsAcknowledged())
			hs.hosts_acknowledged++;
	}

	return hs;
}

/*
 * 'perfdata' must be a flat dictionary with double values
 * 'status' dictionary can contain multiple levels of dictionaries
 */
std::pair<Dictionary::Ptr, Array::Ptr> CIB::GetFeatureStats()
{
	Dictionary::Ptr status = new Dictionary();
	Array::Ptr perfdata = new Array();

	Dictionary::Ptr statsFunctions = ScriptGlobal::Get("StatsFunctions", &Empty);

	if (statsFunctions) {
		ObjectLock olock(statsFunctions);

		for (const Dictionary::Pair& kv : statsFunctions)
			static_cast<Function::Ptr>(kv.second)->Invoke({ status, perfdata });
	}

	return std::make_pair(status, perfdata);
}

REGISTER_STATSFUNCTION(CIB, &CIB::StatsFunc);

void CIB::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata) {
	double interval = Utility::GetTime() - Application::GetStartTime();

	if (interval > 60)
		interval = 60;

	status->Set("active_host_checks", GetActiveHostChecksStatistics(interval) / interval);
	status->Set("passive_host_checks", GetPassiveHostChecksStatistics(interval) / interval);
	status->Set("active_host_checks_1min", GetActiveHostChecksStatistics(60));
	status->Set("passive_host_checks_1min", GetPassiveHostChecksStatistics(60));
	status->Set("active_host_checks_5min", GetActiveHostChecksStatistics(60 * 5));
	status->Set("passive_host_checks_5min", GetPassiveHostChecksStatistics(60 * 5));
	status->Set("active_host_checks_15min", GetActiveHostChecksStatistics(60 * 15));
	status->Set("passive_host_checks_15min", GetPassiveHostChecksStatistics(60 * 15));

	status->Set("active_service_checks", GetActiveServiceChecksStatistics(interval) / interval);
	status->Set("passive_service_checks", GetPassiveServiceChecksStatistics(interval) / interval);
	status->Set("active_service_checks_1min", GetActiveServiceChecksStatistics(60));
	status->Set("passive_service_checks_1min", GetPassiveServiceChecksStatistics(60));
	status->Set("active_service_checks_5min", GetActiveServiceChecksStatistics(60 * 5));
	status->Set("passive_service_checks_5min", GetPassiveServiceChecksStatistics(60 * 5));
	status->Set("active_service_checks_15min", GetActiveServiceChecksStatistics(60 * 15));
	status->Set("passive_service_checks_15min", GetPassiveServiceChecksStatistics(60 * 15));

	CheckableCheckStatistics scs = CalculateServiceCheckStats();

	status->Set("min_latency", scs.min_latency);
	status->Set("max_latency", scs.max_latency);
	status->Set("avg_latency", scs.avg_latency);
	status->Set("min_execution_time", scs.min_execution_time);
	status->Set("max_execution_time", scs.max_execution_time);
	status->Set("avg_execution_time", scs.avg_execution_time);

	ServiceStatistics ss = CalculateServiceStats();

	status->Set("num_services_ok", ss.services_ok);
	status->Set("num_services_warning", ss.services_warning);
	status->Set("num_services_critical", ss.services_critical);
	status->Set("num_services_unknown", ss.services_unknown);
	status->Set("num_services_pending", ss.services_pending);
	status->Set("num_services_unreachable", ss.services_unreachable);
	status->Set("num_services_flapping", ss.services_flapping);
	status->Set("num_services_in_downtime", ss.services_in_downtime);
	status->Set("num_services_acknowledged", ss.services_acknowledged);

	double uptime = Utility::GetTime() - Application::GetStartTime();
	status->Set("uptime", uptime);

	HostStatistics hs = CalculateHostStats();

	status->Set("num_hosts_up", hs.hosts_up);
	status->Set("num_hosts_down", hs.hosts_down);
	status->Set("num_hosts_pending", hs.hosts_pending);
	status->Set("num_hosts_unreachable", hs.hosts_unreachable);
	status->Set("num_hosts_flapping", hs.hosts_flapping);
	status->Set("num_hosts_in_downtime", hs.hosts_in_downtime);
	status->Set("num_hosts_acknowledged", hs.hosts_acknowledged);
}
