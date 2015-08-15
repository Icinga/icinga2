/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"
#include <boost/foreach.hpp>

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
	return m_ActiveHostChecksStatistics.GetValues(timespan);
}

int CIB::GetActiveServiceChecksStatistics(long timespan)
{
	return m_ActiveServiceChecksStatistics.GetValues(timespan);
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
	return m_PassiveHostChecksStatistics.GetValues(timespan);
}

int CIB::GetPassiveServiceChecksStatistics(long timespan)
{
	return m_PassiveServiceChecksStatistics.GetValues(timespan);
}

CheckableCheckStatistics CIB::CalculateHostCheckStats(void)
{
	double min_latency = -1, max_latency = 0, sum_latency = 0;
	int count_latency = 0;
	double min_execution_time = -1, max_execution_time = 0, sum_execution_time = 0;
	int count_execution_time = 0;

	BOOST_FOREACH(const Host::Ptr& host, ConfigType::GetObjectsByType<Host>()) {
		ObjectLock olock(host);

		CheckResult::Ptr cr = host->GetLastCheckResult();

		/* latency */
		double latency = Host::CalculateLatency(cr);

		if (min_latency == -1 || latency < min_latency)
			min_latency = latency;

		if (latency > max_latency)
			max_latency = latency;

		sum_latency += latency;
		count_latency++;

		/* execution_time */
		double execution_time = Host::CalculateExecutionTime(cr);

		if (min_execution_time == -1 || execution_time < min_execution_time)
			min_execution_time = execution_time;

		if (execution_time > max_execution_time)
			max_execution_time = execution_time;

		sum_execution_time += execution_time;
		count_execution_time++;
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

CheckableCheckStatistics CIB::CalculateServiceCheckStats(void)
{
	double min_latency = -1, max_latency = 0, sum_latency = 0;
	int count_latency = 0;
	double min_execution_time = -1, max_execution_time = 0, sum_execution_time = 0;
	int count_execution_time = 0;

	BOOST_FOREACH(const Service::Ptr& service, ConfigType::GetObjectsByType<Service>()) {
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

	CheckableCheckStatistics ccs;

	ccs.min_latency = min_latency;
	ccs.max_latency = max_latency;
	ccs.avg_latency = sum_latency / count_latency;
	ccs.min_execution_time = min_execution_time;
	ccs.max_execution_time = max_execution_time;
	ccs.avg_execution_time = sum_execution_time / count_execution_time;

	return ccs;
}

ServiceStatistics CIB::CalculateServiceStats(void)
{
	ServiceStatistics ss = {0};

	BOOST_FOREACH(const Service::Ptr& service, ConfigType::GetObjectsByType<Service>()) {
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

HostStatistics CIB::CalculateHostStats(void)
{
	HostStatistics hs = {0};

	BOOST_FOREACH(const Host::Ptr& host, ConfigType::GetObjectsByType<Host>()) {
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
std::pair<Dictionary::Ptr, Array::Ptr> CIB::GetFeatureStats(void)
{
	Dictionary::Ptr status = new Dictionary();
	Array::Ptr perfdata = new Array();

	String name;
	BOOST_FOREACH(tie(name, boost::tuples::ignore), StatsFunctionRegistry::GetInstance()->GetItems()) {
		StatsFunction::Ptr func = StatsFunctionRegistry::GetInstance()->GetItem(name);

		if (!func)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Function '" + name + "' does not exist."));

		func->Invoke(status, perfdata);
	}

	return std::make_pair(status, perfdata);
}

