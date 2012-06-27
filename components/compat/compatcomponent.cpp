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
	fp << "hoststatus {" << endl
	   << "\t" << "host_name=" << host.GetName() << endl
	   << "\t" << "has_been_checked=1" << endl
	   << "\t" << "should_be_scheduled=1" << endl
	   << "\t" << "check_execution_time=0" << endl
	   << "\t" << "check_latency=0" << endl
	   << "\t" << "current_state=0" << endl
	   << "\t" << "state_type=1" << endl
	   << "\t" << "last_check=" << time(NULL) << endl
	   << "\t" << "next_check=" << time(NULL) << endl
	   << "\t" << "current_attempt=1" << endl
	   << "\t" << "max_attempts=1" << endl
	   << "\t" << "active_checks_enabled=1" << endl
	   << "\t" << "passive_checks_enabled=1" << endl
	   << "\t" << "}" << endl
	   << endl;
}

void CompatComponent::DumpHostObject(ofstream& fp, Host host)
{
	fp << "define host {" << endl
	   << "\t" << "host_name" << "\t" << host.GetName() << endl
	   << "\t" << "hostgroups" << "\t" << "all-hosts" << endl
	   << "\t" << "check_interval" << "\t" << 1 << endl
	   << "\t" << "retry_interval" << "\t" << 1 << endl
	   << "\t" << "max_check_attempts" << "\t" << 1 << endl
	   << "\t" << "active_checks_enabled" << "\t" << 1 << endl
	   << "\t" << "passive_checks_enabled" << "\t" << 1 << endl
	   << "\t" << "}" << endl
	   << endl;
}

void CompatComponent::DumpServiceStatus(ofstream& fp, Service service)
{
	Dictionary::Ptr cr;
	cr = service.GetLastCheckResult();

	string plugin_output;
	long schedule_start = -1, schedule_end = -1;
	long execution_start = -1, execution_end = -1;
	if (cr) {
		cr->GetProperty("output", &plugin_output);
		cr->GetProperty("schedule_start", &schedule_start);
		cr->GetProperty("schedule_end", &schedule_end);
		cr->GetProperty("execution_start", &execution_start);
		cr->GetProperty("execution_end", &execution_end);
	}

	long execution_time = (execution_end - execution_start);
	long latency = (schedule_end - schedule_start) - execution_time;

	fp << "servicestatus {" << endl
           << "\t" << "host_name=" << service.GetHost().GetName() << endl
	   << "\t" << "service_description=" << service.GetDisplayName() << endl
	   << "\t" << "check_interval=" << service.GetCheckInterval() / 60.0 << endl
	   << "\t" << "retry_interval=" << service.GetRetryInterval() / 60.0 << endl
	   << "\t" << "has_been_checked=" << (cr ? 1 : 0) << endl
	   << "\t" << "should_be_scheduled=1" << endl
	   << "\t" << "check_execution_time=" << execution_time << endl
	   << "\t" << "check_latency=" << latency << endl
	   << "\t" << "current_state=" << service.GetState() << endl
	   << "\t" << "state_type=" << service.GetStateType() << endl
	   << "\t" << "plugin_output=" << plugin_output << endl
	   << "\t" << "last_check=" << schedule_start << endl
	   << "\t" << "next_check=" << service.GetNextCheck() << endl
	   << "\t" << "current_attempt=" << service.GetCurrentCheckAttempt() << endl
	   << "\t" << "max_attempts=" << service.GetMaxCheckAttempts() << endl
	   << "\t" << "last_state_change=" << service.GetLastStateChange() << endl
	   << "\t" << "last_hard_state_change=" << service.GetLastHardStateChange() << endl
	   << "\t" << "last_update=" << time(NULL) << endl
	   << "\t" << "active_checks_enabled=1" << endl
	   << "\t" << "passive_checks_enabled=1" << endl
	   << "\t" << "}" << endl
	   << endl;
}

void CompatComponent::DumpServiceObject(ofstream& fp, Service service)
{
	fp << "define service {" << endl
	   << "\t" << "host_name" << "\t" << service.GetHost().GetName() << endl
	   << "\t" << "service_description" << "\t" << service.GetDisplayName() << endl
	   << "\t" << "check_command" << "\t" << "check_i2" << endl
	   << "\t" << "check_interval" << "\t" << service.GetCheckInterval() / 60.0 << endl
	   << "\t" << "retry_interval" << "\t" << service.GetRetryInterval() / 60.0 << endl
	   << "\t" << "max_check_attempts" << "\t" << 1 << endl
	   << "\t" << "active_checks_enabled" << "\t" << 1 << endl
	   << "\t" << "passive_checks_enabled" << "\t" << 1 << endl
	   << "\t" << "}" << endl
	   << endl;
}

/**
 * Periodically writes the status.dat and objects.cache files.
 */
void CompatComponent::StatusTimerHandler(void)
{
	ofstream statusfp;
	statusfp.open("status.dat.tmp", ofstream::out | ofstream::trunc);

	statusfp << "# Icinga status file" << endl
		 << "# This file is auto-generated. Do not modify this file." << endl
		 << endl;

	statusfp << "info {" << endl
		 << "\t" << "created=" << time(NULL) << endl
		 << "\t" << "version=2.0" << endl
		 << "\t" << "}" << endl
		 << endl;

	statusfp << "programstatus {" << endl
		 << "\t" << "daemon_mode=1" << endl
		 << "\t" << "program_start=" << IcingaApplication::GetInstance()->GetStartTime() << endl
		 << "\t" << "active_service_checks_enabled=1" << endl
		 << "\t" << "passive_service_checks_enabled=1" << endl
		 << "\t" << "active_host_checks_enabled=0" << endl
		 << "\t" << "passive_host_checks_enabled=0" << endl
		 << "\t" << "check_service_freshness=1" << endl
		 << "\t" << "check_host_freshness=0" << endl
		 << "\t" << "enable_flap_detection=1" << endl
		 << "\t" << "enable_failure_prediction=0" << endl
		 << endl;

	ofstream objectfp;
	objectfp.open("objects.cache.tmp", ofstream::out | ofstream::trunc);

	objectfp << "# Icinga object cache file" << endl
		 << "# This file is auto-generated. Do not modify this file." << endl
		 << endl;

	objectfp << "define hostgroup {" << endl
		 << "\t" << "hostgroup_name" << "\t" << "all-hosts" << endl;

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
		objectfp << endl;
	}

	objectfp << "\t" << "}" << endl
		 << endl;

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
