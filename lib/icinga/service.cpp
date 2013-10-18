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
#include "config/configitembuilder.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/utility.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/bind/apply.hpp>

using namespace icinga;

REGISTER_TYPE(Service);

boost::signals2::signal<void (const Service::Ptr&, const String&, const String&, AcknowledgementType, double, const String&)> Service::OnAcknowledgementSet;
boost::signals2::signal<void (const Service::Ptr&, const String&)> Service::OnAcknowledgementCleared;

Service::Service(void)
	: m_CheckRunning(false)
{ }

void Service::Start(void)
{
	DynamicObject::Start();

	VERIFY(GetHost());

	SetSchedulingOffset(Utility::Random());
	UpdateNextCheck();

	AddDowntimesToCache();
	AddCommentsToCache();

	StartDowntimesExpiredTimer();
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

	Host::Ptr host = GetHost();
	if (host)
		host->AddService(GetSelf());

	UpdateSlaveNotifications();
}

String Service::GetDisplayName(void) const
{
	if (m_DisplayName.IsEmpty())
		return GetShortName();
	else
		return m_DisplayName;
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
	return Host::GetByName(m_HostName);
}

Dictionary::Ptr Service::GetMacros(void) const
{
	return m_Macros;
}

Array::Ptr Service::GetHostDependencies(void) const
{
	return m_HostDependencies;
}

Array::Ptr Service::GetServiceDependencies(void) const
{
	return m_ServiceDependencies;
}

Array::Ptr Service::GetGroups(void) const
{
	return m_Groups;
}

String Service::GetHostName(void) const
{
	return m_HostName;
}

String Service::GetShortName(void) const
{
	if (m_ShortName.IsEmpty())
		return GetName();
	else
		return m_ShortName;
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

bool Service::IsVolatile(void) const
{
	if (m_Volatile.IsEmpty())
		return false;

	return m_Volatile;
}

AcknowledgementType Service::GetAcknowledgement(void)
{
	ASSERT(OwnsLock());

	if (m_Acknowledgement.IsEmpty())
		return AcknowledgementNone;

	int ivalue = static_cast<int>(m_Acknowledgement);
	AcknowledgementType avalue = static_cast<AcknowledgementType>(ivalue);

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

double Service::GetAcknowledgementExpiry(void) const
{
	if (m_AcknowledgementExpiry.IsEmpty())
		return 0;

	return static_cast<double>(m_AcknowledgementExpiry);
}

void Service::AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, double expiry, const String& authority)
{
	{
		ObjectLock olock(this);

		m_Acknowledgement = type;
		m_AcknowledgementExpiry = expiry;
	}

	OnNotificationsRequested(GetSelf(), NotificationAcknowledgement, GetLastCheckResult(), author, comment);

	boost::function<void (void)> f = boost::bind(boost::ref(Service::OnAcknowledgementSet), GetSelf(), author, comment, type, expiry, authority);
	Utility::QueueAsyncCallback(f);
}

void Service::ClearAcknowledgement(const String& authority)
{
	ASSERT(OwnsLock());

	m_Acknowledgement = AcknowledgementNone;
	m_AcknowledgementExpiry = 0;

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnAcknowledgementCleared), GetSelf(), authority));
}

std::set<Host::Ptr> Service::GetParentHosts(void) const
{
	std::set<Host::Ptr> parents;

	Host::Ptr host = GetHost();

	/* The service's host is implicitly a parent. */
	if (host)
		parents.insert(host);

	Array::Ptr dependencies = GetHostDependencies();

	if (dependencies) {
		ObjectLock olock(dependencies);

		BOOST_FOREACH(const Value& dependency, dependencies) {
			Host::Ptr host = Host::GetByName(dependency);

			if (!host)
				continue;

			parents.insert(host);
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

			if (!service)
				continue;

			if (service->GetName() == GetName())
				continue;

			parents.insert(service);
		}
	}

	return parents;
}

bool Service::GetEnablePerfdata(void) const
{
	if (!m_EnablePerfdata.IsEmpty())
		return m_EnablePerfdata;
	else
		return true;
}

int Service::GetModifiedAttributes(void) const
{
	int attrs = 0;

	if (!m_OverrideEnableActiveChecks.IsEmpty())
		attrs |= ModAttrActiveChecksEnabled;

	if (!m_OverrideEnablePassiveChecks.IsEmpty())
		attrs |= ModAttrPassiveChecksEnabled;

	if (!m_OverrideEnableEventHandler.IsEmpty())
		attrs |= ModAttrEventHandlerEnabled;

	if (!m_OverrideCheckInterval.IsEmpty())
		attrs |= ModAttrNormalCheckInterval;

	if (!m_OverrideRetryInterval.IsEmpty())
		attrs |= ModAttrRetryCheckInterval;

	// TODO: finish

	return attrs;
}

void Service::SetModifiedAttributes(int flags)
{
	if ((flags & ModAttrActiveChecksEnabled) == 0)
		m_OverrideEnableActiveChecks = Empty;

	if ((flags & ModAttrPassiveChecksEnabled) == 0)
		m_OverrideEnablePassiveChecks = Empty;

	if ((flags & ModAttrEventHandlerEnabled) == 0)
		m_OverrideEnableEventHandler = Empty;

	if ((flags & ModAttrNormalCheckInterval) == 0)
		m_OverrideCheckInterval = Empty;

	if ((flags & ModAttrRetryCheckInterval) == 0)
		m_OverrideRetryInterval = Empty;
}

bool Service::ResolveMacro(const String& macro, const Dictionary::Ptr& cr, String *result) const
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
		*result = Convert::ToString(GetCurrentCheckAttempt());
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
			*result = cr->Get("output");
			return true;
		} else if (macro == "SERVICEPERFDATA") {
			*result = cr->Get("performance_data_raw");
			return true;
		} else if (macro == "LASTSERVICECHECK") {
			*result = Convert::ToString((long)cr->Get("execution_end"));
			return true;
		}
	}

	Dictionary::Ptr macros = GetMacros();

	if (macros && macros->Contains(macro)) {
		*result = macros->Get(macro);
		return true;
	}

	return false;
}

void Service::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("display_name", m_DisplayName);
		bag->Set("macros", m_Macros);
		bag->Set("host_dependencies", m_HostDependencies);
		bag->Set("service_dependencies", m_ServiceDependencies);
		bag->Set("groups", m_Groups);
		bag->Set("check_command", m_CheckCommand);
		bag->Set("max_check_attempts", m_MaxCheckAttempts);
		bag->Set("check_period", m_CheckPeriod);
		bag->Set("check_interval", m_CheckInterval);
		bag->Set("retry_interval", m_RetryInterval);
		bag->Set("event_command", m_EventCommand);
		bag->Set("volatile", m_Volatile);
		bag->Set("short_name", m_ShortName);
		bag->Set("host", m_HostName);
		bag->Set("flapping_threshold", m_FlappingThreshold);
		bag->Set("notifications", m_NotificationDescriptions);
		bag->Set("enable_active_checks", m_EnableActiveChecks);
		bag->Set("enable_passive_checks", m_EnablePassiveChecks);
		bag->Set("enable_event_handler", m_EnableEventHandler);
	}

	if (attributeTypes & Attribute_State) {
		bag->Set("next_check", m_NextCheck);
		bag->Set("current_checker", m_CurrentChecker);
		bag->Set("check_attempt", m_CheckAttempt);
		bag->Set("state", m_State);
		bag->Set("state_type", m_StateType);
		bag->Set("last_state", m_LastState);
		bag->Set("last_hard_state", m_LastHardState);
		bag->Set("last_state_type", m_LastStateType);
		bag->Set("last_reachable", m_LastReachable);
		bag->Set("last_result", m_LastResult);
		bag->Set("last_state_change", m_LastStateChange);
		bag->Set("last_hard_state_change", m_LastHardStateChange);
		bag->Set("last_state_ok", m_LastStateOK);
		bag->Set("last_state_warning", m_LastStateWarning);
		bag->Set("last_state_critical", m_LastStateCritical);
		bag->Set("last_state_unknown", m_LastStateUnknown);
		bag->Set("last_state_unreachable", m_LastStateUnreachable);
		bag->Set("last_in_downtime", m_LastInDowntime);
		bag->Set("enable_active_checks", m_EnableActiveChecks);
		bag->Set("enable_passive_checks", m_EnablePassiveChecks);
		bag->Set("force_next_check", m_ForceNextCheck);
		bag->Set("acknowledgement", m_Acknowledgement);
		bag->Set("acknowledgement_expiry", m_AcknowledgementExpiry);
		bag->Set("comments", m_Comments);
		bag->Set("downtimes", m_Downtimes);
		bag->Set("enable_notifications", m_EnableNotifications);
		bag->Set("force_next_notification", m_ForceNextNotification);
		bag->Set("flapping_positive", m_FlappingPositive);
		bag->Set("flapping_negative", m_FlappingNegative);
		bag->Set("flapping_lastchange", m_FlappingLastChange);
		bag->Set("enable_flapping", m_EnableFlapping);
		bag->Set("enable_perfdata", m_EnablePerfdata);
		bag->Set("override_enable_active_checks", m_OverrideEnableActiveChecks);
		bag->Set("override_enable_passive_checks", m_OverrideEnablePassiveChecks);
		bag->Set("override_check_interval", m_OverrideCheckInterval);
		bag->Set("override_retry_interval", m_OverrideRetryInterval);
		bag->Set("override_enable_event_handler", m_OverrideEnableEventHandler);
	}
}

void Service::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_DisplayName = bag->Get("display_name");
		m_Macros = bag->Get("macros");
		m_HostDependencies = bag->Get("host_dependencies");
		m_ServiceDependencies = bag->Get("service_dependencies");
		m_Groups = bag->Get("groups");
		m_CheckCommand = bag->Get("check_command");
		m_MaxCheckAttempts = bag->Get("max_check_attempts");
		m_CheckPeriod = bag->Get("check_period");
		m_CheckInterval = bag->Get("check_interval");
		m_RetryInterval = bag->Get("retry_interval");
		m_EventCommand = bag->Get("event_command");
		m_Volatile = bag->Get("volatile");
		m_ShortName = bag->Get("short_name");
		m_HostName = bag->Get("host");
		m_FlappingThreshold = bag->Get("flapping_threshold");
		m_NotificationDescriptions = bag->Get("notifications");
		m_EnableActiveChecks = bag->Get("enable_active_checks");
		m_EnablePassiveChecks = bag->Get("enable_passive_checks");
		m_EnableEventHandler = bag->Get("enable_event_handler");
	}

	if (attributeTypes & Attribute_State) {
		m_NextCheck = bag->Get("next_check");
		m_CurrentChecker = bag->Get("current_checker");
		m_CheckAttempt = bag->Get("check_attempt");
		m_State = bag->Get("state");
		m_StateType = bag->Get("state_type");
		m_LastState = bag->Get("last_state");
		m_LastHardState = bag->Get("last_hard_state");
		m_LastStateType = bag->Get("last_state_type");
		m_LastReachable = bag->Get("last_reachable");
		m_LastResult = bag->Get("last_result");
		m_LastStateChange = bag->Get("last_state_change");
		m_LastHardStateChange = bag->Get("last_hard_state_change");
		m_LastStateOK = bag->Get("last_state_ok");
		m_LastStateWarning = bag->Get("last_state_warning");
		m_LastStateCritical = bag->Get("last_state_critical");
		m_LastStateUnknown = bag->Get("last_state_unknown");
		m_LastStateUnreachable = bag->Get("last_state_unreachable");
		m_LastInDowntime = bag->Get("last_in_downtime");
		m_ForceNextCheck = bag->Get("force_next_check");
		m_Acknowledgement = bag->Get("acknowledgement");
		m_AcknowledgementExpiry = bag->Get("acknowledgement_expiry");
		m_Comments = bag->Get("comments");
		m_Downtimes = bag->Get("downtimes");
		m_EnableNotifications = bag->Get("enable_notifications");
		m_ForceNextNotification = bag->Get("force_next_notification");
		m_FlappingPositive = bag->Get("flapping_positive");
		m_FlappingNegative = bag->Get("flapping_negative");
		m_FlappingLastChange = bag->Get("flapping_lastchange");
		m_EnableFlapping = bag->Get("enable_flapping");
		m_EnablePerfdata = bag->Get("enable_perfdata");
		m_OverrideEnableActiveChecks = bag->Get("override_enable_active_checks");
		m_OverrideEnablePassiveChecks = bag->Get("override_enable_passive_checks");
		m_OverrideCheckInterval = bag->Get("override_check_interval");
		m_OverrideRetryInterval = bag->Get("override_retry_interval");
		m_OverrideEnableEventHandler = bag->Get("override_enable_event_handler");
	}
}
