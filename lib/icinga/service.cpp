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

#include "icinga/service.h"
#include "icinga/servicegroup.h"
#include "icinga/checkcommand.h"
#include "icinga/icingaapplication.h"
#include "icinga/macroprocessor.h"
#include "icinga/pluginutility.h"
#include "icinga/dependency.h"
#include "config/configitembuilder.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/initialize.h"
#include <boost/foreach.hpp>
#include <boost/bind/apply.hpp>

using namespace icinga;

REGISTER_TYPE(Service);

INITIALIZE_ONCE(&Service::StartDowntimesExpiredTimer);

void Service::OnConfigLoaded(void)
{
	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			ServiceGroup::Ptr sg = ServiceGroup::GetByName(name);

			if (sg)
				sg->AddMember(GetSelf());
		}
	}

	m_Host = Host::GetByName(GetHostRaw());

	if (m_Host)
		m_Host->AddService(GetSelf());

	SetSchedulingOffset(Utility::Random());

	Checkable::OnConfigLoaded();
}

Service::Ptr Service::GetByNamePair(const String& hostName, const String& serviceName)
{
	if (!hostName.IsEmpty()) {
		Host::Ptr host = Host::GetByName(hostName);

		if (!host)
			return Service::Ptr();

		return host->GetServiceByShortName(serviceName);
	} else {
		return Service::GetByName(serviceName);
	}
}

Host::Ptr Service::GetHost(void) const
{
	return m_Host;
}

ServiceState Service::StateFromString(const String& state)
{
	if (state == "OK")
		return StateOK;
	else if (state == "WARNING")
		return StateWarning;
	else if (state == "CRITICAL")
		return StateCritical;
	else
		return StateUnknown;
}

String Service::StateToString(ServiceState state)
{
	switch (state) {
		case StateOK:
			return "OK";
		case StateWarning:
			return "WARNING";
		case StateCritical:
			return "CRITICAL";
		case StateUnknown:
		default:
			return "UNKNOWN";
	}
}

StateType Service::StateTypeFromString(const String& type)
{
	if (type == "SOFT")
		return StateTypeSoft;
	else
		return StateTypeHard;
}

String Service::StateTypeToString(StateType type)
{
	if (type == StateTypeSoft)
		return "SOFT";
	else
		return "HARD";
}

bool Service::ResolveMacro(const String& macro, const CheckResult::Ptr& cr, String *result) const
{
	if (macro == "SERVICEDESC") {
		*result = GetShortName();
		return true;
	} else if (macro == "SERVICEDISPLAYNAME") {
		*result = GetDisplayName();
		return true;
	} else if (macro == "SERVICECHECKCOMMAND") {
		CheckCommand::Ptr commandObj = GetCheckCommand();

		if (commandObj)
			*result = commandObj->GetName();
		else
			*result = "";

		return true;
	}

	if (macro == "SERVICESTATE") {
		*result = StateToString(GetState());
		return true;
	} else if (macro == "SERVICESTATEID") {
		*result = Convert::ToString(GetState());
		return true;
	} else if (macro == "SERVICESTATETYPE") {
		*result = StateTypeToString(GetStateType());
		return true;
	} else if (macro == "SERVICEATTEMPT") {
		*result = Convert::ToString(GetCheckAttempt());
		return true;
	} else if (macro == "MAXSERVICEATTEMPT") {
		*result = Convert::ToString(GetMaxCheckAttempts());
		return true;
	} else if (macro == "LASTSERVICESTATE") {
		*result = StateToString(GetLastState());
		return true;
	} else if (macro == "LASTSERVICESTATEID") {
		*result = Convert::ToString(GetLastState());
		return true;
	} else if (macro == "LASTSERVICESTATETYPE") {
		*result = StateTypeToString(GetLastStateType());
		return true;
	} else if (macro == "LASTSERVICESTATECHANGE") {
		*result = Convert::ToString((long)GetLastStateChange());
		return true;
	} else if (macro == "SERVICEDURATIONSEC") {
		*result = Convert::ToString((long)(Utility::GetTime() - GetLastStateChange()));
		return true;
	} else if (macro == "TOTALHOSTSERVICES" || macro == "TOTALHOSTSERVICESOK" || macro == "TOTALHOSTSERVICESWARNING"
	    || macro == "TOTALHOSTSERVICESUNKNOWN" || macro == "TOTALHOSTSERVICESCRITICAL") {
		int filter = -1;
		int count = 0;

		if (macro == "TOTALHOSTSERVICESOK")
			filter = StateOK;
		else if (macro == "TOTALHOSTSERVICESWARNING")
			filter = StateWarning;
		else if (macro == "TOTALHOSTSERVICESUNKNOWN")
			filter = StateUnknown;
		else if (macro == "TOTALHOSTSERVICESCRITICAL")
			filter = StateCritical;

		BOOST_FOREACH(const Service::Ptr& service, GetHost()->GetServices()) {
			if (filter != -1 && service->GetState() != filter)
				continue;

			count++;
		}

		*result = Convert::ToString(count);
		return true;
	}

	if (cr) {
		if (macro == "SERVICELATENCY") {
			*result = Convert::ToString(Service::CalculateLatency(cr));
			return true;
		} else if (macro == "SERVICEEXECUTIONTIME") {
			*result = Convert::ToString(Service::CalculateExecutionTime(cr));
			return true;
		} else if (macro == "SERVICEOUTPUT") {
			*result = cr->GetOutput();
			return true;
		} else if (macro == "SERVICEPERFDATA") {
			*result = PluginUtility::FormatPerfdata(cr->GetPerformanceData());
			return true;
		} else if (macro == "LASTSERVICECHECK") {
			*result = Convert::ToString((long)cr->GetExecutionEnd());
			return true;
		}
	}

	Dictionary::Ptr vars = GetVars();

	if (macro.SubStr(0, 8) == "_SERVICE") {
		*result = vars ? vars->Get(macro.SubStr(8)) : "";
		return true;
	}


	if (vars && vars->Contains(macro)) {
		*result = vars->Get(macro);
		return true;
	}

	return false;
}

boost::tuple<Host::Ptr, Service::Ptr> icinga::GetHostService(const Checkable::Ptr& checkable)
{
	Service::Ptr service = dynamic_pointer_cast<Service>(checkable);

	if (service)
		return boost::make_tuple(service->GetHost(), service);
	else
		return boost::make_tuple(static_pointer_cast<Host>(checkable), Service::Ptr());
}

