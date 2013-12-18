/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

boost::signals2::signal<void (const Service::Ptr&, const String&, const String&, AcknowledgementType, double, const String&)> Service::OnAcknowledgementSet;
boost::signals2::signal<void (const Service::Ptr&, const String&)> Service::OnAcknowledgementCleared;

INITIALIZE_ONCE(&Service::StartDowntimesExpiredTimer);

Service::Service(void)
	: m_CheckRunning(false)
{ }

void Service::Start(void)
{
	double now = Utility::GetTime();

	if (GetNextCheck() < now + 300)
		SetNextCheck(now + Utility::Random() % 300);

	DynamicObject::Start();
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

	m_Host = Host::GetByName(GetHostRaw());

	if (m_Host)
		m_Host->AddService(GetSelf());

	UpdateSlaveNotifications();
	UpdateSlaveScheduledDowntimes();

	SetSchedulingOffset(Utility::Random());
}

void Service::OnStateLoaded(void)
{
	AddDowntimesToCache();
	AddCommentsToCache();

	std::vector<String> ids;
	Dictionary::Ptr downtimes = GetDowntimes();

	{
		ObjectLock dlock(downtimes);
		BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
			Downtime::Ptr downtime = kv.second;

			if (downtime->GetScheduledBy().IsEmpty())
				continue;

			ids.push_back(kv.first);
		}
	}

	BOOST_FOREACH(const String& id, ids) {
		RemoveDowntime(id, true);
	}
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

bool Service::IsHostCheck(void) const
{
	ASSERT(!OwnsLock());

	Service::Ptr hc = GetHost()->GetCheckService();

	if (!hc)
		return false;

	return (hc->GetName() == GetName());

}

bool Service::IsReachable(void) const
{
	ASSERT(!OwnsLock());

	BOOST_FOREACH(const Service::Ptr& service, GetParentServices()) {
		/* ignore ourselves */
		if (service->GetName() == GetName())
			continue;

		ObjectLock olock(service);

		/* ignore pending services */
		if (!service->GetLastCheckResult())
			continue;

		/* ignore soft states */
		if (service->GetStateType() == StateTypeSoft)
			continue;

		/* ignore services states OK and Warning */
		if (service->GetState() == StateOK ||
		    service->GetState() == StateWarning)
			continue;

		return false;
	}

	BOOST_FOREACH(const Host::Ptr& host, GetParentHosts()) {
		Service::Ptr hc = host->GetCheckService();

		/* ignore hosts that don't have a hostcheck */
		if (!hc)
			continue;

		/* ignore ourselves */
		if (hc->GetName() == GetName())
			continue;

		ObjectLock olock(hc);

		/* ignore soft states */
		if (hc->GetStateType() == StateTypeSoft)
			continue;

		/* ignore hosts that are up */
		if (hc->GetState() == StateOK)
			continue;

		return false;
	}

	return true;
}

AcknowledgementType Service::GetAcknowledgement(void)
{
	ASSERT(OwnsLock());

	AcknowledgementType avalue = static_cast<AcknowledgementType>(GetAcknowledgementRaw());

	if (avalue != AcknowledgementNone) {
		double expiry = GetAcknowledgementExpiry();

		if (expiry != 0 && expiry < Utility::GetTime()) {
			avalue = AcknowledgementNone;
			ClearAcknowledgement();
		}
	}

	return avalue;
}

bool Service::IsAcknowledged(void)
{
	return GetAcknowledgement() != AcknowledgementNone;
}

void Service::AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, double expiry, const String& authority)
{
	{
		ObjectLock olock(this);

		SetAcknowledgementRaw(type);
		SetAcknowledgementExpiry(expiry);
	}

	OnNotificationsRequested(GetSelf(), NotificationAcknowledgement, GetLastCheckResult(), author, comment);

	OnAcknowledgementSet(GetSelf(), author, comment, type, expiry, authority);
}

void Service::ClearAcknowledgement(const String& authority)
{
	ASSERT(OwnsLock());

	SetAcknowledgementRaw(AcknowledgementNone);
	SetAcknowledgementExpiry(0);

	OnAcknowledgementCleared(GetSelf(), authority);
}

std::set<Host::Ptr> Service::GetParentHosts(void) const
{
	std::set<Host::Ptr> parents;

	Host::Ptr host = GetHost();

	/* The service's host is implicitly a parent. */
	parents.insert(host);

	Array::Ptr dependencies = GetHostDependencies();

	if (dependencies) {
		ObjectLock olock(dependencies);

		BOOST_FOREACH(const String& dependency, dependencies) {
			parents.insert(Host::GetByName(dependency));
		}
	}

	return parents;
}

std::set<Service::Ptr> Service::GetParentServices(void) const
{
	std::set<Service::Ptr> parents;

	Host::Ptr host = GetHost();
	Array::Ptr dependencies = GetServiceDependencies();

	if (host && dependencies) {
		ObjectLock olock(dependencies);

		BOOST_FOREACH(const Value& dependency, dependencies) {
			Service::Ptr service = host->GetServiceByShortName(dependency);

			if (!service || service->GetName() == GetName())
				continue;

			parents.insert(service);
		}
	}

	return parents;
}

bool Service::GetEnablePerfdata(void) const
{
	if (!GetOverrideEnablePerfdata().IsEmpty())
		return GetOverrideEnablePerfdata();
	else
		return GetEnablePerfdataRaw();
}

void Service::SetEnablePerfdata(bool enabled, const String& authority)
{
	SetOverrideEnablePerfdata(enabled);
}

int Service::GetModifiedAttributes(void) const
{
	int attrs = 0;

	if (!GetOverrideEnableNotifications().IsEmpty())
		attrs |= ModAttrNotificationsEnabled;

	if (!GetOverrideEnableActiveChecks().IsEmpty())
		attrs |= ModAttrActiveChecksEnabled;

	if (!GetOverrideEnablePassiveChecks().IsEmpty())
		attrs |= ModAttrPassiveChecksEnabled;

	if (!GetOverrideEnableFlapping().IsEmpty())
		attrs |= ModAttrFlapDetectionEnabled;

	if (!GetOverrideEnableEventHandler().IsEmpty())
		attrs |= ModAttrEventHandlerEnabled;

	if (!GetOverrideEnablePerfdata().IsEmpty())
		attrs |= ModAttrPerformanceDataEnabled;

	if (!GetOverrideCheckInterval().IsEmpty())
		attrs |= ModAttrNormalCheckInterval;

	if (!GetOverrideRetryInterval().IsEmpty())
		attrs |= ModAttrRetryCheckInterval;

	if (!GetOverrideEventCommand().IsEmpty())
		attrs |= ModAttrEventHandlerCommand;

	if (!GetOverrideCheckCommand().IsEmpty())
		attrs |= ModAttrCheckCommand;

	if (!GetOverrideMaxCheckAttempts().IsEmpty())
		attrs |= ModAttrMaxCheckAttempts;

	if (!GetOverrideCheckPeriod().IsEmpty())
		attrs |= ModAttrCheckTimeperiod;

	// TODO: finish

	return attrs;
}

void Service::SetModifiedAttributes(int flags)
{
	if ((flags & ModAttrNotificationsEnabled) == 0)
		SetOverrideEnableNotifications(Empty);

	if ((flags & ModAttrActiveChecksEnabled) == 0)
		SetOverrideEnableActiveChecks(Empty);

	if ((flags & ModAttrPassiveChecksEnabled) == 0)
		SetOverrideEnablePassiveChecks(Empty);

	if ((flags & ModAttrFlapDetectionEnabled) == 0)
		SetOverrideEnableFlapping(Empty);

	if ((flags & ModAttrEventHandlerEnabled) == 0)
		SetOverrideEnableEventHandler(Empty);

	if ((flags & ModAttrPerformanceDataEnabled) == 0)
		SetOverrideEnablePerfdata(Empty);

	if ((flags & ModAttrNormalCheckInterval) == 0)
		SetOverrideCheckInterval(Empty);

	if ((flags & ModAttrRetryCheckInterval) == 0)
		SetOverrideRetryInterval(Empty);

	if ((flags & ModAttrEventHandlerCommand) == 0)
		SetOverrideEventCommand(Empty);

	if ((flags & ModAttrCheckCommand) == 0)
		SetOverrideCheckCommand(Empty);

	if ((flags & ModAttrMaxCheckAttempts) == 0)
		SetOverrideMaxCheckAttempts(Empty);

	if ((flags & ModAttrCheckTimeperiod) == 0)
		SetOverrideCheckPeriod(Empty);
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

	if (macro.SubStr(0, 8) == "_SERVICE") {
		Dictionary::Ptr custom = GetCustom();
		*result = custom ? custom->Get(macro.SubStr(8)) : "";
		return true;
	}

	Dictionary::Ptr macros = GetMacros();

	if (macros && macros->Contains(macro)) {
		*result = macros->Get(macro);
		return true;
	}

	return false;
}
