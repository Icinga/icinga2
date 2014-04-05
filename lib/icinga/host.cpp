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

#include "icinga/host.h"
#include "icinga/service.h"
#include "icinga/hostgroup.h"
#include "icinga/icingaapplication.h"
#include "icinga/pluginutility.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/scriptfunction.h"
#include "base/debug.h"
#include "base/serializer.h"
#include "config/configitembuilder.h"
#include "config/configcompilercontext.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Host);

void Host::OnConfigLoaded(void)
{
	DynamicObject::OnConfigLoaded();

	ASSERT(!OwnsLock());

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			if (hg)
				hg->AddMember(GetSelf());
		}
	}
}

void Host::Stop(void)
{
	DynamicObject::Stop();

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			if (hg)
				hg->RemoveMember(GetSelf());
		}
	}

	// TODO: unregister slave services/notifications?
}

std::set<Service::Ptr> Host::GetServices(void) const
{
	boost::mutex::scoped_lock lock(m_ServicesMutex);

	std::set<Service::Ptr> services;
	typedef std::pair<String, Service::Ptr> ServicePair;
	BOOST_FOREACH(const ServicePair& kv, m_Services) {
		services.insert(kv.second);
	}

	return services;
}

void Host::AddService(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_ServicesMutex);

	m_Services[service->GetShortName()] = service;
}

void Host::RemoveService(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_ServicesMutex);

	m_Services.erase(service->GetShortName());
}

int Host::GetTotalServices(void) const
{
	return GetServices().size();
}

Service::Ptr Host::GetServiceByShortName(const Value& name)
{
	if (name.IsScalar()) {
		{
			boost::mutex::scoped_lock lock(m_ServicesMutex);

			std::map<String, Service::Ptr>::const_iterator it = m_Services.find(name);

			if (it != m_Services.end())
				return it->second;
		}

		return Service::Ptr();
	} else if (name.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = name;
		String short_name;

		return Service::GetByNamePair(dict->Get("host"), dict->Get("service"));
	} else {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Host/Service name pair is invalid: " + JsonSerialize(name)));
	}
}

HostState Host::CalculateState(ServiceState state, bool reachable)
{
	if (!reachable)
		return HostUnreachable;

	switch (state) {
		case StateOK:
		case StateWarning:
			return HostUp;
		default:
			return HostDown;
	}
}

HostState Host::GetState(void) const
{
	ASSERT(!OwnsLock());

	if (!IsReachable())
		return HostUnreachable;

	switch (GetStateRaw()) {
		case StateOK:
		case StateWarning:
			return HostUp;
		default:
			return HostDown;
	}

}

HostState Host::GetLastState(void) const
{
	ASSERT(!OwnsLock());

	if (!IsReachable())
		return HostUnreachable;

	switch (GetLastStateRaw()) {
		case StateOK:
		case StateWarning:
			return HostUp;
		default:
			return HostDown;
	}
}

HostState Host::GetLastHardState(void) const
{
	ASSERT(!OwnsLock());

	if (!IsReachable())
		return HostUnreachable;

	switch (GetLastHardStateRaw()) {
		case StateOK:
		case StateWarning:
			return HostUp;
		default:
			return HostDown;
	}
}

double Host::GetLastStateUp(void) const
{
	ASSERT(!OwnsLock());

	if (GetLastStateOK() > GetLastStateWarning())
		return GetLastStateOK();
	else
		return GetLastStateWarning();
}

double Host::GetLastStateDown(void) const
{
	ASSERT(!OwnsLock());

	return GetLastStateCritical();
}

HostState Host::StateFromString(const String& state)
{
	if (state == "UP")
		return HostUp;
	else if (state == "DOWN")
		return HostDown;
	else if (state == "UNREACHABLE")
		return HostUnreachable;
	else
		return HostUnreachable;
}

String Host::StateToString(HostState state)
{
	switch (state) {
		case HostUp:
			return "UP";
		case HostDown:
			return "DOWN";
		case HostUnreachable:
			return "UNREACHABLE";
		default:
			return "INVALID";
	}
}

StateType Host::StateTypeFromString(const String& type)
{
	if (type == "SOFT")
		return StateTypeSoft;
	else
		return StateTypeHard;
}

String Host::StateTypeToString(StateType type)
{
	if (type == StateTypeSoft)
		return "SOFT";
	else
		return "HARD";
}

bool Host::ResolveMacro(const String& macro, const CheckResult::Ptr&, String *result) const
{
	/* require prefix for object macros */
	if (macro.SubStr(0, 5) == "host.") {
		String key = macro.SubStr(5);

		if (key == "name") {
			*result = GetName();
			return true;
		}
		else if (key == "displaymane") {
			*result = GetDisplayName();
			return true;
		}

		CheckResult::Ptr cr = GetLastCheckResult();

		if (key == "state") {
			switch (GetState()) {
				case HostUnreachable:
					*result = "UNREACHABLE";
					break;
				case HostUp:
					*result = "UP";
					break;
				case HostDown:
					*result = "DOWN";
					break;
				default:
					ASSERT(0);
			}

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
		} else if (key == "checkcommand") {
			CheckCommand::Ptr commandObj = GetCheckCommand();

			if (commandObj)
				*result = commandObj->GetName();
			else
				*result = "";

			return true;
		} else if (key == "totalservices" || key == "totalservicesok" || key == "totalserviceswarning"
			    || key == "totalservicesunknown" || key == "totalservicescritical") {
				int filter = -1;
				int count = 0;

				if (key == "totalservicesok")
					filter = StateOK;
				else if (key == "totalserviceswarning")
					filter = StateWarning;
				else if (key == "totalservicesunknown")
					filter = StateUnknown;
				else if (key == "totalservicescritical")
					filter = StateCritical;

				BOOST_FOREACH(const Service::Ptr& service, GetServices()) {
					if (filter != -1 && service->GetState() != filter)
						continue;

					count++;
				}

				*result = Convert::ToString(count);
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
				*result = Convert::ToString((long)cr->GetScheduleStart());
				return true;
			}
		}
	}

	Dictionary::Ptr vars = GetVars();

	if (vars && vars->Contains(macro)) {
		*result = vars->Get(macro);
		return true;
	}

	return false;
}
