/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-compat.h"

using namespace icinga;

/**
 * Hint: The reason why we're using "\n" rather than std::endl is because
 * std::endl also _flushes_ the output stream which severely degrades
 * performance (see http://gcc.gnu.org/onlinedocs/libstdc++/manual/bk01pt11ch25s02.html).
 */

/**
 * Returns the name of the component.
 *
 * @returns The name.
 */
string CompatComponent::GetName(void) const
{
	return "compat";
}

/**
 * Starts the component.
 */
void CompatComponent::Start(void)
{
	m_StatusTimer = boost::make_shared<Timer>();
	m_StatusTimer->SetInterval(15);
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&CompatComponent::StatusTimerHandler, this));
	m_StatusTimer->Start();

	CIB::RequireInformation(CIB_ProgramStatus);
	CIB::RequireInformation(CIB_ServiceStatus);
}

/**
 * Stops the component.
 */
void CompatComponent::Stop(void)
{
}

void CompatComponent::DumpHostStatus(ofstream& fp, Host host)
{
	fp << "hoststatus {" << "\n"
	   << "\t" << "host_name=" << host.GetName() << "\n"
	   << "\t" << "has_been_checked=1" << "\n"
	   << "\t" << "should_be_scheduled=1" << "\n"
	   << "\t" << "check_execution_time=0" << "\n"
	   << "\t" << "check_latency=0" << "\n"
	   << "\t" << "current_state=0" << "\n"
	   << "\t" << "state_type=1" << "\n"
	   << "\t" << "last_check=" << time(NULL) << "\n"
	   << "\t" << "next_check=" << time(NULL) << "\n"
	   << "\t" << "current_attempt=1" << "\n"
	   << "\t" << "max_attempts=1" << "\n"
	   << "\t" << "active_checks_enabled=1" << "\n"
	   << "\t" << "passive_checks_enabled=1" << "\n"
	   << "\t" << "}" << "\n"
	   << "\n";
}

void CompatComponent::DumpHostObject(ofstream& fp, Host host)
{
	fp << "define host {" << "\n"
	   << "\t" << "host_name" << "\t" << host.GetName() << "\n"
	   << "\t" << "hostgroups" << "\t" << "all-hosts" << "\n"
	   << "\t" << "check_interval" << "\t" << 1 << "\n"
	   << "\t" << "retry_interval" << "\t" << 1 << "\n"
	   << "\t" << "max_check_attempts" << "\t" << 1 << "\n"
	   << "\t" << "active_checks_enabled" << "\t" << 1 << "\n"
	   << "\t" << "passive_checks_enabled" << "\t" << 1 << "\n"
	   << "\t" << "}" << "\n"
	   << "\n";
}

void CompatComponent::DumpServiceStatus(ofstream& fp, Service service)
{
	Dictionary::Ptr cr;
	cr = service.GetLastCheckResult();

	string output;
	string perfdata;
	long schedule_start = -1, schedule_end = -1;
	long execution_start = -1, execution_end = -1;
	if (cr) {
		cr->GetProperty("output", &output);
		cr->GetProperty("schedule_start", &schedule_start);
		cr->GetProperty("schedule_end", &schedule_end);
		cr->GetProperty("execution_start", &execution_start);
		cr->GetProperty("execution_end", &execution_end);
		cr->GetProperty("performance_data_raw", &perfdata);
	}

	long execution_time = (execution_end - execution_start);
	long latency = (schedule_end - schedule_start) - execution_time;

	fp << "servicestatus {" << "\n"
           << "\t" << "host_name=" << service.GetHost().GetName() << "\n"
	   << "\t" << "service_description=" << service.GetAlias() << "\n"
	   << "\t" << "check_interval=" << service.GetCheckInterval() / 60.0 << "\n"
	   << "\t" << "retry_interval=" << service.GetRetryInterval() / 60.0 << "\n"
	   << "\t" << "has_been_checked=" << (cr ? 1 : 0) << "\n"
	   << "\t" << "should_be_scheduled=1" << "\n"
	   << "\t" << "check_execution_time=" << execution_time << "\n"
	   << "\t" << "check_latency=" << latency << "\n"
	   << "\t" << "current_state=" << service.GetState() << "\n"
	   << "\t" << "state_type=" << service.GetStateType() << "\n"
	   << "\t" << "plugin_output=" << output << "\n"
	   << "\t" << "performance_data=" << perfdata << "\n"
	   << "\t" << "last_check=" << schedule_end << "\n"
	   << "\t" << "next_check=" << service.GetNextCheck() << "\n"
	   << "\t" << "current_attempt=" << service.GetCurrentCheckAttempt() << "\n"
	   << "\t" << "max_attempts=" << service.GetMaxCheckAttempts() << "\n"
	   << "\t" << "last_state_change=" << service.GetLastStateChange() << "\n"
	   << "\t" << "last_hard_state_change=" << service.GetLastHardStateChange() << "\n"
	   << "\t" << "last_update=" << time(NULL) << "\n"
	   << "\t" << "active_checks_enabled=1" << "\n"
	   << "\t" << "passive_checks_enabled=1" << "\n"
	   << "\t" << "}" << "\n"
	   << "\n";
}

void CompatComponent::DumpServiceObject(ofstream& fp, Service service)
{
	fp << "define service {" << "\n"
	   << "\t" << "host_name" << "\t" << service.GetHost().GetName() << "\n"
	   << "\t" << "service_description" << "\t" << service.GetAlias() << "\n"
	   << "\t" << "check_command" << "\t" << "check_i2" << "\n"
	   << "\t" << "check_interval" << "\t" << service.GetCheckInterval() / 60.0 << "\n"
	   << "\t" << "retry_interval" << "\t" << service.GetRetryInterval() / 60.0 << "\n"
	   << "\t" << "max_check_attempts" << "\t" << 1 << "\n"
	   << "\t" << "active_checks_enabled" << "\t" << 1 << "\n"
	   << "\t" << "passive_checks_enabled" << "\t" << 1 << "\n"
	   << "\t" << "}" << "\n"
	   << "\n";
}

/**
 * Periodically writes the status.dat and objects.cache files.
 */
void CompatComponent::StatusTimerHandler(void)
{
	ofstream statusfp;
	statusfp.open("status.dat.tmp", ofstream::out | ofstream::trunc);

	statusfp << "# Icinga status file" << "\n"
		 << "# This file is auto-generated. Do not modify this file." << "\n"
		 << "\n";

	statusfp << "info {" << "\n"
		 << "\t" << "created=" << time(NULL) << "\n"
		 << "\t" << "version=2.0" << "\n"
		 << "\t" << "}" << "\n"
		 << "\n";

	statusfp << "programstatus {" << "\n"
		 << "\t" << "daemon_mode=1" << "\n"
		 << "\t" << "program_start=" << IcingaApplication::GetInstance()->GetStartTime() << "\n"
		 << "\t" << "active_service_checks_enabled=1" << "\n"
		 << "\t" << "passive_service_checks_enabled=1" << "\n"
		 << "\t" << "active_host_checks_enabled=0" << "\n"
		 << "\t" << "passive_host_checks_enabled=0" << "\n"
		 << "\t" << "check_service_freshness=1" << "\n"
		 << "\t" << "check_host_freshness=0" << "\n"
		 << "\t" << "enable_flap_detection=1" << "\n"
		 << "\t" << "enable_failure_prediction=0" << "\n"
		 << "\t" << "active_scheduled_service_check_stats=" << CIB::GetTaskStatistics(60) << "," << CIB::GetTaskStatistics(5 * 60) << "," << CIB::GetTaskStatistics(15 * 60) << "\n"
		 << "\t" << "}" << "\n"
		 << "\n";

	ofstream objectfp;
	objectfp.open("objects.cache.tmp", ofstream::out | ofstream::trunc);

	objectfp << "# Icinga object cache file" << "\n"
		 << "# This file is auto-generated. Do not modify this file." << "\n"
		 << "\n";

	objectfp << "define hostgroup {" << "\n"
		 << "\t" << "hostgroup_name" << "\t" << "all-hosts" << "\n";

	ConfigObject::TMap::Range range;
	range = ConfigObject::GetObjects("host");

	ConfigObject::TMap::Iterator it;

	if (range.first != range.second) {
		objectfp << "\t" << "members" << "\t";
		for (it = range.first; it != range.second; it++) {
			Host host(it->second);

			objectfp << host.GetName();

			if (distance(it, range.second) != 1)
				objectfp << ",";
		}
		objectfp << "\n";
	}

	objectfp << "\t" << "}" << "\n"
		 << "\n";

	for (it = range.first; it != range.second; it++) {
		DumpHostStatus(statusfp, it->second);
		DumpHostObject(objectfp, it->second);
	}

	range = ConfigObject::GetObjects("service");

	for (it = range.first; it != range.second; it++) {
		DumpServiceStatus(statusfp, it->second);
		DumpServiceObject(objectfp, it->second);
	}

	statusfp.close();
	rename("status.dat.tmp", "status.dat");

	objectfp.close();
	rename("objects.cache.tmp", "objects.cache");
}

EXPORT_COMPONENT(compat, CompatComponent);
