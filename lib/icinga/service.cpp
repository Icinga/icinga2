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

String ServiceNameComposer::MakeName(const String& shortName, const Dictionary::Ptr props) const {
	if (!props)
		return "";

	return props->Get("host_name") + "!" + shortName;
}

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

	m_Host = Host::GetByName(GetHostName());

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
		return ServiceOK;
	else if (state == "WARNING")
		return ServiceWarning;
	else if (state == "CRITICAL")
		return ServiceCritical;
	else
		return ServiceUnknown;
}

String Service::StateToString(ServiceState state)
{
	switch (state) {
		case ServiceOK:
			return "OK";
		case ServiceWarning:
			return "WARNING";
		case ServiceCritical:
			return "CRITICAL";
		case ServiceUnknown:
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
	String key;
	Dictionary::Ptr vars;

	/* require prefix for object macros */
	if (macro.SubStr(0, 8) == "service.") {
		key = macro.SubStr(8);

		if (key.SubStr(0, 5) == "vars.") {
			vars = GetVars();
			String vars_key = key.SubStr(5);

			if (vars && vars->Contains(vars_key)) {
				*result = vars->Get(vars_key);
				return true;
			}
		} else if (key == "description") {
			*result = GetShortName();
			return true;
		} else if (key == "displayname") {
			*result = GetDisplayName();
			return true;
		} else if (key == "checkcommand") {
			CheckCommand::Ptr commandObj = GetCheckCommand();

			if (commandObj)
				*result = commandObj->GetName();
			else
				*result = "";

			return true;
		}

		if (key == "state") {
			*result = StateToString(GetState());
			return true;
		} else if (key == "stateid") {
			*result = Convert::ToString(GetState());
			return true;
		} else if (key == "statetype") {
			*result = StateTypeToString(GetStateType());
			return true;
		} else if (key == "attempt") {
			*result = Convert::ToString(GetCheckAttempt());
			return true;
		} else if (key == "maxattempt") {
			*result = Convert::ToString(GetMaxCheckAttempts());
			return true;
		} else if (key == "laststate") {
			*result = StateToString(GetLastState());
			return true;
		} else if (key == "laststateid") {
			*result = Convert::ToString(GetLastState());
			return true;
		} else if (key == "laststatetype") {
			*result = StateTypeToString(GetLastStateType());
			return true;
		} else if (key == "laststatechange") {
			*result = Convert::ToString((long)GetLastStateChange());
			return true;
		} else if (key == "durationsec") {
			*result = Convert::ToString((long)(Utility::GetTime() - GetLastStateChange()));
			return true;
		}

		if (cr) {
			if (key == "latency") {
				*result = Convert::ToString(Service::CalculateLatency(cr));
				return true;
			} else if (key == "executiontime") {
				*result = Convert::ToString(Service::CalculateExecutionTime(cr));
				return true;
			} else if (key == "output") {
				*result = cr->GetOutput();
				return true;
			} else if (key == "perfdata") {
				*result = PluginUtility::FormatPerfdata(cr->GetPerformanceData());
				return true;
			} else if (key == "lastcheck") {
				*result = Convert::ToString((long)cr->GetExecutionEnd());
				return true;
			}
		}
	} else {
		vars = GetVars();

		if (vars && vars->Contains(macro)) {
			*result = vars->Get(macro);
			return true;
		}
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

