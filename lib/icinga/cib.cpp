/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/cib.h"
#include "icinga/service.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/dynamictype.h"
#include "base/statsfunction.h"
#include <boost/foreach.hpp>

using namespace icinga;

RingBuffer CIB::m_ActiveChecksStatistics(15 * 60);
RingBuffer CIB::m_PassiveChecksStatistics(15 * 60);

void CIB::UpdateActiveChecksStatistics(long tv, int num)
{
	m_ActiveChecksStatistics.InsertValue(tv, num);
}

int CIB::GetActiveChecksStatistics(long timespan)
{
	return m_ActiveChecksStatistics.GetValues(timespan);
}

void CIB::UpdatePassiveChecksStatistics(long tv, int num)
{
	m_PassiveChecksStatistics.InsertValue(tv, num);
}

int CIB::GetPassiveChecksStatistics(long timespan)
{
	return m_PassiveChecksStatistics.GetValues(timespan);
}

ServiceCheckStatistics CIB::CalculateServiceCheckStats(void)
{
	double min_latency = -1, max_latency = 0, sum_latency = 0;
	int count_latency = 0;
	double min_execution_time = -1, max_execution_time = 0, sum_execution_time = 0;
	int count_execution_time = 0;

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
	}

	ServiceCheckStatistics scs = {0};

	scs.min_latency = min_latency;
	scs.max_latency = max_latency;
	scs.avg_latency = sum_latency / count_latency;
	scs.min_execution_time = min_execution_time;
	scs.max_execution_time = max_execution_time;
	scs.avg_execution_time = sum_execution_time / count_execution_time;

	return scs;
}

ServiceStatistics CIB::CalculateServiceStats(void)
{
	ServiceStatistics ss = {0};

	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		ObjectLock olock(service);

		CheckResult::Ptr cr = service->GetLastCheckResult();

		if (service->GetState() == StateOK)
			ss.services_ok++;
		if (service->GetState() == StateWarning)
			ss.services_warning++;
		if (service->GetState() == StateCritical)
			ss.services_critical++;
		if (service->GetState() == StateUnknown)
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

HostStatistics CIB::CalculateHostStats(void)
{
	HostStatistics hs = {0};

	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
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
std::pair<Dictionary::Ptr, Dictionary::Ptr> CIB::GetFeatureStats(void)
{
	Dictionary::Ptr status = make_shared<Dictionary>();
	Dictionary::Ptr perfdata = make_shared<Dictionary>();

	String name;
	Value ret;
	BOOST_FOREACH(tie(name, boost::tuples::ignore), StatsFunctionRegistry::GetInstance()->GetItems()) {
		StatsFunction::Ptr func = StatsFunctionRegistry::GetInstance()->GetItem(name);

		if (!func)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Function '" + name + "' does not exist."));

		ret = func->Invoke(status, perfdata);
	}

	return std::make_pair(status, perfdata);
}

