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

#include "icinga/icingastatuswriter.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include "icinga/hostgroup.h"
#include "icinga/servicegroup.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/timeperiod.h"
#include "icinga/notificationcommand.h"
#include "icinga/compatutility.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include "base/application.h"
#include "base/context.h"
#include "base/statsfunction.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>

using namespace icinga;

REGISTER_TYPE(IcingaStatusWriter);

REGISTER_STATSFUNCTION(IcingaStatusWriterStats, &IcingaStatusWriter::StatsFunc);

Value IcingaStatusWriter::StatsFunc(Dictionary::Ptr& status, Dictionary::Ptr& perfdata)
{
	/* FIXME */
	status->Set("icingastatuswriter_", 1);

	return 0;
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

	m_StatusTimer = make_shared<Timer>();
	m_StatusTimer->SetInterval(GetUpdateInterval());
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&IcingaStatusWriter::StatusTimerHandler, this));
	m_StatusTimer->Start();
	m_StatusTimer->Reschedule(0);
}

Dictionary::Ptr IcingaStatusWriter::GetStatusData(void)
{
	Dictionary::Ptr bag = make_shared<Dictionary>();

	/* features */
	std::pair<Dictionary::Ptr, Dictionary::Ptr> stats = CIB::GetFeatureStats();

	bag->Set("feature_status", stats.first);
	bag->Set("feature_perfdata", stats.second);

	/* icinga stats */
	double interval = Utility::GetTime() - Application::GetStartTime();

	if (interval > 60)
		interval = 60;

	bag->Set("active_checks", CIB::GetActiveChecksStatistics(interval) / interval);
	bag->Set("passive_checks", CIB::GetPassiveChecksStatistics(interval) / interval);

	bag->Set("active_checks_1min", CIB::GetActiveChecksStatistics(60));
	bag->Set("passive_checks_1min", CIB::GetPassiveChecksStatistics(60));
	bag->Set("active_checks_5min", CIB::GetActiveChecksStatistics(60 * 5));
	bag->Set("passive_checks_5min", CIB::GetPassiveChecksStatistics(60 * 5));
	bag->Set("active_checks_15min", CIB::GetActiveChecksStatistics(60 * 15));
	bag->Set("passive_checks_15min", CIB::GetPassiveChecksStatistics(60 * 15));

	ServiceCheckStatistics scs = CIB::CalculateServiceCheckStats();

	bag->Set("min_latency", scs.min_latency);
	bag->Set("max_latency", scs.max_latency);
	bag->Set("avg_latency", scs.avg_latency);
	bag->Set("min_execution_time", scs.min_latency);
	bag->Set("max_execution_time", scs.max_latency);
	bag->Set("avg_execution_time", scs.avg_execution_time);

	ServiceStatistics ss = CIB::CalculateServiceStats();

	bag->Set("num_services_ok", ss.services_ok);
	bag->Set("num_services_warning", ss.services_warning);
	bag->Set("num_services_critical", ss.services_critical);
	bag->Set("num_services_unknown", ss.services_unknown);
	bag->Set("num_services_pending", ss.services_pending);
	bag->Set("num_services_unreachable", ss.services_unreachable);
	bag->Set("num_services_flapping", ss.services_flapping);
	bag->Set("num_services_in_downtime", ss.services_in_downtime);
	bag->Set("num_services_acknowledged", ss.services_acknowledged);

	HostStatistics hs = CIB::CalculateHostStats();

	bag->Set("num_hosts_up", hs.hosts_up);
	bag->Set("num_hosts_down", hs.hosts_down);
	bag->Set("num_hosts_unreachable", hs.hosts_unreachable);
	bag->Set("num_hosts_flapping", hs.hosts_flapping);
	bag->Set("num_hosts_in_downtime", hs.hosts_in_downtime);
	bag->Set("num_hosts_acknowledged", hs.hosts_acknowledged);

	return bag;
}


void IcingaStatusWriter::StatusTimerHandler(void)
{
	Log(LogInformation, "icinga", "Writing status.json file");

        String statuspath = GetStatusPath();
        String statuspathtmp = statuspath + ".tmp"; /* XXX make this a global definition */

        std::ofstream statusfp;
        statusfp.open(statuspathtmp.CStr(), std::ofstream::out | std::ofstream::trunc);

        statusfp << std::fixed;

	statusfp << JsonSerialize(GetStatusData());

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

        Log(LogInformation, "icinga", "Finished writing status.json file");
}

