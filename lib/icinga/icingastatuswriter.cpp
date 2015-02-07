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

#include "icinga/icingastatuswriter.hpp"
#include "icinga/cib.hpp"
#include "base/dynamictype.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"
#include "base/statsfunction.hpp"
#include "base/json.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>

using namespace icinga;

REGISTER_TYPE(IcingaStatusWriter);

REGISTER_STATSFUNCTION(IcingaStatusWriterStats, &IcingaStatusWriter::StatsFunc);

void IcingaStatusWriter::StatsFunc(Dictionary::Ptr& status, Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	BOOST_FOREACH(const IcingaStatusWriter::Ptr& icingastatuswriter, DynamicType::GetObjectsByType<IcingaStatusWriter>()) {
		nodes->Set(icingastatuswriter->GetName(), 1); //add more stats
	}

	status->Set("icingastatuswriter", nodes);
}

/**
 * Hint: The reason why we're using "\n" rather than std::endl is because
 * std::endl also _flushes_ the output stream which severely degrades
 * performance (see http://gcc.gnu.org/onlinedocs/libstdc++/manual/bk01pt11ch25s02.html).
 */

/**
 * Starts the component.
 */
void IcingaStatusWriter::Start(void)
{
	DynamicObject::Start();

	m_StatusTimer = new Timer();
	m_StatusTimer->SetInterval(GetUpdateInterval());
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&IcingaStatusWriter::StatusTimerHandler, this));
	m_StatusTimer->Start();
	m_StatusTimer->Reschedule(0);
}

Dictionary::Ptr IcingaStatusWriter::GetStatusData(void)
{
	Dictionary::Ptr bag = new Dictionary();

	/* features */
	std::pair<Dictionary::Ptr, Array::Ptr> stats = CIB::GetFeatureStats();

	bag->Set("feature_status", stats.first);
	bag->Set("feature_perfdata", stats.second);

	/* icinga stats */
	Dictionary::Ptr icinga_stats = new Dictionary();

	double interval = Utility::GetTime() - Application::GetStartTime();

	if (interval > 60)
		interval = 60;

	icinga_stats->Set("active_host_checks", CIB::GetActiveHostChecksStatistics(interval) / interval);
	icinga_stats->Set("passive_host_checks", CIB::GetPassiveHostChecksStatistics(interval) / interval);
	icinga_stats->Set("active_host_checks_1min", CIB::GetActiveHostChecksStatistics(60));
	icinga_stats->Set("passive_host_checks_1min", CIB::GetPassiveHostChecksStatistics(60));
	icinga_stats->Set("active_host_checks_5min", CIB::GetActiveHostChecksStatistics(60 * 5));
	icinga_stats->Set("passive_host_checks_5min", CIB::GetPassiveHostChecksStatistics(60 * 5));
	icinga_stats->Set("active_host_checks_15min", CIB::GetActiveHostChecksStatistics(60 * 15));
	icinga_stats->Set("passive_host_checks_15min", CIB::GetPassiveHostChecksStatistics(60 * 15));

	icinga_stats->Set("active_service_checks", CIB::GetActiveServiceChecksStatistics(interval) / interval);
	icinga_stats->Set("passive_service_checks", CIB::GetPassiveServiceChecksStatistics(interval) / interval);
	icinga_stats->Set("active_service_checks_1min", CIB::GetActiveServiceChecksStatistics(60));
	icinga_stats->Set("passive_service_checks_1min", CIB::GetPassiveServiceChecksStatistics(60));
	icinga_stats->Set("active_service_checks_5min", CIB::GetActiveServiceChecksStatistics(60 * 5));
	icinga_stats->Set("passive_service_checks_5min", CIB::GetPassiveServiceChecksStatistics(60 * 5));
	icinga_stats->Set("active_service_checks_15min", CIB::GetActiveServiceChecksStatistics(60 * 15));
	icinga_stats->Set("passive_service_checks_15min", CIB::GetPassiveServiceChecksStatistics(60 * 15));

	CheckableCheckStatistics scs = CIB::CalculateServiceCheckStats();

	icinga_stats->Set("min_latency", scs.min_latency);
	icinga_stats->Set("max_latency", scs.max_latency);
	icinga_stats->Set("avg_latency", scs.avg_latency);
	icinga_stats->Set("min_execution_time", scs.min_latency);
	icinga_stats->Set("max_execution_time", scs.max_latency);
	icinga_stats->Set("avg_execution_time", scs.avg_execution_time);

	ServiceStatistics ss = CIB::CalculateServiceStats();

	icinga_stats->Set("num_services_ok", ss.services_ok);
	icinga_stats->Set("num_services_warning", ss.services_warning);
	icinga_stats->Set("num_services_critical", ss.services_critical);
	icinga_stats->Set("num_services_unknown", ss.services_unknown);
	icinga_stats->Set("num_services_pending", ss.services_pending);
	icinga_stats->Set("num_services_unreachable", ss.services_unreachable);
	icinga_stats->Set("num_services_flapping", ss.services_flapping);
	icinga_stats->Set("num_services_in_downtime", ss.services_in_downtime);
	icinga_stats->Set("num_services_acknowledged", ss.services_acknowledged);

	HostStatistics hs = CIB::CalculateHostStats();

	icinga_stats->Set("num_hosts_up", hs.hosts_up);
	icinga_stats->Set("num_hosts_down", hs.hosts_down);
	icinga_stats->Set("num_hosts_unreachable", hs.hosts_unreachable);
	icinga_stats->Set("num_hosts_flapping", hs.hosts_flapping);
	icinga_stats->Set("num_hosts_in_downtime", hs.hosts_in_downtime);
	icinga_stats->Set("num_hosts_acknowledged", hs.hosts_acknowledged);

	bag->Set("icinga_status", icinga_stats);

	return bag;
}


void IcingaStatusWriter::StatusTimerHandler(void)
{
	Log(LogNotice, "IcingaStatusWriter", "Writing status.json file");

	String statuspath = GetStatusPath();
	String statuspathtmp = statuspath + ".tmp"; /* XXX make this a global definition */

	std::ofstream statusfp;
	statusfp.open(statuspathtmp.CStr(), std::ofstream::out | std::ofstream::trunc);

	statusfp << std::fixed;

	statusfp << JsonEncode(GetStatusData());

	statusfp.close();

#ifdef _WIN32
	_unlink(statuspath.CStr());
#endif /* _WIN32 */

	if (rename(statuspathtmp.CStr(), statuspath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(statuspathtmp));
	}

	Log(LogNotice, "IcingaStatusWriter", "Finished writing status.json file");
}

