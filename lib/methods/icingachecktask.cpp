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

#include "methods/icingachecktask.hpp"
#include "icinga/cib.hpp"
#include "icinga/service.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/perfdatavalue.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/function.hpp"
#include "base/configtype.hpp"

using namespace icinga;

REGISTER_SCRIPTFUNCTION(IcingaCheck, &IcingaCheckTask::ScriptFunc);

void IcingaCheckTask::ScriptFunc(const Checkable::Ptr& service, const CheckResult::Ptr& cr,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	if (resolvedMacros && !useResolvedMacros)
		return;

	double interval = Utility::GetTime() - Application::GetStartTime();

	if (interval > 60)
		interval = 60;

	Array::Ptr perfdata = new Array();

	perfdata->Add(new PerfdataValue("active_host_checks", CIB::GetActiveHostChecksStatistics(interval) / interval));
	perfdata->Add(new PerfdataValue("passive_host_checks", CIB::GetPassiveHostChecksStatistics(interval) / interval));
	perfdata->Add(new PerfdataValue("active_host_checks_1min", CIB::GetActiveHostChecksStatistics(60)));
	perfdata->Add(new PerfdataValue("passive_host_checks_1min", CIB::GetPassiveHostChecksStatistics(60)));
	perfdata->Add(new PerfdataValue("active_host_checks_5min", CIB::GetActiveHostChecksStatistics(60 * 5)));
	perfdata->Add(new PerfdataValue("passive_host_checks_5min", CIB::GetPassiveHostChecksStatistics(60 * 5)));
	perfdata->Add(new PerfdataValue("active_host_checks_15min", CIB::GetActiveHostChecksStatistics(60 * 15)));
	perfdata->Add(new PerfdataValue("passive_host_checks_15min", CIB::GetPassiveHostChecksStatistics(60 * 15)));

	perfdata->Add(new PerfdataValue("active_service_checks", CIB::GetActiveServiceChecksStatistics(interval) / interval));
	perfdata->Add(new PerfdataValue("passive_service_checks", CIB::GetPassiveServiceChecksStatistics(interval) / interval));
	perfdata->Add(new PerfdataValue("active_service_checks_1min", CIB::GetActiveServiceChecksStatistics(60)));
	perfdata->Add(new PerfdataValue("passive_service_checks_1min", CIB::GetPassiveServiceChecksStatistics(60)));
	perfdata->Add(new PerfdataValue("active_service_checks_5min", CIB::GetActiveServiceChecksStatistics(60 * 5)));
	perfdata->Add(new PerfdataValue("passive_service_checks_5min", CIB::GetPassiveServiceChecksStatistics(60 * 5)));
	perfdata->Add(new PerfdataValue("active_service_checks_15min", CIB::GetActiveServiceChecksStatistics(60 * 15)));
	perfdata->Add(new PerfdataValue("passive_service_checks_15min", CIB::GetPassiveServiceChecksStatistics(60 * 15)));

	CheckableCheckStatistics scs = CIB::CalculateServiceCheckStats();

	perfdata->Add(new PerfdataValue("min_latency", scs.min_latency));
	perfdata->Add(new PerfdataValue("max_latency", scs.max_latency));
	perfdata->Add(new PerfdataValue("avg_latency", scs.avg_latency));
	perfdata->Add(new PerfdataValue("min_execution_time", scs.min_latency));
	perfdata->Add(new PerfdataValue("max_execution_time", scs.max_latency));
	perfdata->Add(new PerfdataValue("avg_execution_time", scs.avg_execution_time));

	ServiceStatistics ss = CIB::CalculateServiceStats();

	perfdata->Add(new PerfdataValue("num_services_ok", ss.services_ok));
	perfdata->Add(new PerfdataValue("num_services_warning", ss.services_warning));
	perfdata->Add(new PerfdataValue("num_services_critical", ss.services_critical));
	perfdata->Add(new PerfdataValue("num_services_unknown", ss.services_unknown));
	perfdata->Add(new PerfdataValue("num_services_pending", ss.services_pending));
	perfdata->Add(new PerfdataValue("num_services_unreachable", ss.services_unreachable));
	perfdata->Add(new PerfdataValue("num_services_flapping", ss.services_flapping));
	perfdata->Add(new PerfdataValue("num_services_in_downtime", ss.services_in_downtime));
	perfdata->Add(new PerfdataValue("num_services_acknowledged", ss.services_acknowledged));

	double uptime = Utility::GetTime() - Application::GetStartTime();
	perfdata->Add(new PerfdataValue("uptime", uptime));

	HostStatistics hs = CIB::CalculateHostStats();

	perfdata->Add(new PerfdataValue("num_hosts_up", hs.hosts_up));
	perfdata->Add(new PerfdataValue("num_hosts_down", hs.hosts_down));
	perfdata->Add(new PerfdataValue("num_hosts_pending", hs.hosts_pending));
	perfdata->Add(new PerfdataValue("num_hosts_unreachable", hs.hosts_unreachable));
	perfdata->Add(new PerfdataValue("num_hosts_flapping", hs.hosts_flapping));
	perfdata->Add(new PerfdataValue("num_hosts_in_downtime", hs.hosts_in_downtime));
	perfdata->Add(new PerfdataValue("num_hosts_acknowledged", hs.hosts_acknowledged));

	cr->SetOutput("Icinga 2 has been running for " + Utility::FormatDuration(uptime) +
	    ". Version: " + Application::GetAppVersion());
	cr->SetPerformanceData(perfdata);
	cr->SetState(ServiceOK);

	service->ProcessCheckResult(cr);
}
