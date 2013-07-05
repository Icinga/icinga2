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

#include "icinga/service.h"
#include "icinga/servicegroup.h"
#include "icinga/checkcommand.h"
#include "icinga/icingaapplication.h"
#include "icinga/macroprocessor.h"
#include "icinga/downtimemessage.h"
#include "config/configitembuilder.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/utility.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Service);

Endpoint::Ptr Service::m_Endpoint;
boost::once_flag Service::m_OnceFlag = BOOST_ONCE_INIT;

Service::Service(const Dictionary::Ptr& serializedObject)
	: DynamicObject(serializedObject), m_CheckRunning(false)
{

	RegisterAttribute("display_name", Attribute_Config, &m_DisplayName);
	RegisterAttribute("macros", Attribute_Config, &m_Macros);
	RegisterAttribute("custom", Attribute_Config, &m_Custom);
	RegisterAttribute("hostdependencies", Attribute_Config, &m_HostDependencies);
	RegisterAttribute("servicedependencies", Attribute_Config, &m_ServiceDependencies);
	RegisterAttribute("servicegroups", Attribute_Config, &m_ServiceGroups);

	RegisterAttribute("check_command", Attribute_Config, &m_CheckCommand);
	RegisterAttribute("max_check_attempts", Attribute_Config, &m_MaxCheckAttempts);
	RegisterAttribute("check_period", Attribute_Config, &m_CheckPeriod);
	RegisterAttribute("check_interval", Attribute_Config, &m_CheckInterval);
	RegisterAttribute("retry_interval", Attribute_Config, &m_RetryInterval);
	RegisterAttribute("checkers", Attribute_Config, &m_Checkers);

	RegisterAttribute("event_command", Attribute_Config, &m_EventCommand);
	RegisterAttribute("volatile", Attribute_Config, &m_Volatile);

	RegisterAttribute("next_check", Attribute_Replicated, &m_NextCheck);
	RegisterAttribute("current_checker", Attribute_Replicated, &m_CurrentChecker);
	RegisterAttribute("check_attempt", Attribute_Replicated, &m_CheckAttempt);
	RegisterAttribute("state", Attribute_Replicated, &m_State);
	RegisterAttribute("state_type", Attribute_Replicated, &m_StateType);
	RegisterAttribute("last_state", Attribute_Replicated, &m_LastState);
	RegisterAttribute("last_state_type", Attribute_Replicated, &m_LastStateType);
	RegisterAttribute("last_reachable", Attribute_Replicated, &m_LastReachable);
	RegisterAttribute("last_result", Attribute_Replicated, &m_LastResult);
	RegisterAttribute("last_state_change", Attribute_Replicated, &m_LastStateChange);
	RegisterAttribute("last_hard_state_change", Attribute_Replicated, &m_LastHardStateChange);
	RegisterAttribute("last_in_downtime", Attribute_Replicated, &m_LastInDowntime);
	RegisterAttribute("enable_active_checks", Attribute_Replicated, &m_EnableActiveChecks);
	RegisterAttribute("enable_passive_checks", Attribute_Replicated, &m_EnablePassiveChecks);
	RegisterAttribute("force_next_check", Attribute_Replicated, &m_ForceNextCheck);

	RegisterAttribute("short_name", Attribute_Config, &m_ShortName);
	RegisterAttribute("host_name", Attribute_Config, &m_HostName);

	RegisterAttribute("acknowledgement", Attribute_Replicated, &m_Acknowledgement);
	RegisterAttribute("acknowledgement_expiry", Attribute_Replicated, &m_AcknowledgementExpiry);

	RegisterAttribute("comments", Attribute_Replicated, &m_Comments);

	RegisterAttribute("downtimes", Attribute_Replicated, &m_Downtimes);

	RegisterAttribute("enable_notifications", Attribute_Replicated, &m_EnableNotifications);
	RegisterAttribute("force_next_notification", Attribute_Replicated, &m_ForceNextNotification);

	RegisterAttribute("flapping_positive", Attribute_Replicated, &m_FlappingPositive);
	RegisterAttribute("flapping_negative", Attribute_Replicated, &m_FlappingNegative);
	RegisterAttribute("flapping_lastchange", Attribute_Replicated, &m_FlappingLastChange);
	RegisterAttribute("flapping_threshold", Attribute_Config, &m_FlappingThreshold);
	RegisterAttribute("enable_flapping", Attribute_Replicated, &m_EnableFlapping);

	SetSchedulingOffset(rand());

	boost::call_once(m_OnceFlag, &Service::Initialize);
}

Service::~Service(void)
{
	ServiceGroup::InvalidateMembersCache();
	Host::InvalidateServicesCache();
	Service::InvalidateDowntimesCache();
	Service::InvalidateCommentsCache();
}

void Service::Initialize(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("service", false);
	m_Endpoint->RegisterTopicHandler("icinga::NotificationSent",
	    boost::bind(&Service::NotificationSentRequestHandler, _3));
	m_Endpoint->RegisterTopicHandler("icinga::Downtime",
	    boost::bind(&Service::DowntimeRequestHandler, _3));
	m_Endpoint->RegisterTopicHandler("icinga::Flapping",
	    boost::bind(&Service::FlappingRequestHandler, _3));
}

void Service::OnRegistrationCompleted(void)
{
	ASSERT(!OwnsLock());

	DynamicObject::OnRegistrationCompleted();

	InvalidateNotificationsCache();
}

String Service::GetDisplayName(void) const
{
	if (m_DisplayName.IsEmpty())
		return GetShortName();
	else
		return m_DisplayName;
}

Service::Ptr Service::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Service", name);

	return dynamic_pointer_cast<Service>(configObject);
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

Dictionary::Ptr Service::GetCustom(void) const
{
	return m_Custom;
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
	return m_ServiceGroups;
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
		Service::Ptr hc = host->GetHostCheckService();

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
			SetAcknowledgement(avalue);
			SetAcknowledgementExpiry(0);
		}
	}

	return avalue;
}

void Service::SetAcknowledgement(AcknowledgementType acknowledgement)
{
	m_Acknowledgement = acknowledgement;
	Touch("acknowledgement");
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

void Service::SetAcknowledgementExpiry(double timestamp)
{
	m_AcknowledgementExpiry = timestamp;
	Touch("acknowledgement_expiry");
}

void Service::AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, double expiry)
{
	{
		ObjectLock olock(this);

		SetAcknowledgement(type);
		SetAcknowledgementExpiry(expiry);
	}

	(void) AddComment(CommentAcknowledgement, author, comment, 0);

	RequestNotifications(NotificationAcknowledgement, GetLastCheckResult(), author, comment);
}

void Service::ClearAcknowledgement(void)
{
	ObjectLock olock(this);

	SetAcknowledgement(AcknowledgementNone);
	SetAcknowledgementExpiry(0);
}

void Service::OnAttributeChanged(const String& name)
{
	ASSERT(!OwnsLock());

	Service::Ptr self = GetSelf();

	if (name == "current_checker")
		OnCheckerChanged(self);
	else if (name == "next_check")
		OnNextCheckChanged(self);
	else if (name == "servicegroups")
		ServiceGroup::InvalidateMembersCache();
	else if (name == "host_name" || name == "short_name") {
		Host::InvalidateServicesCache();

		UpdateSlaveNotifications();
	} else if (name == "downtimes")
		Service::InvalidateDowntimesCache();
	else if (name == "comments")
		Service::InvalidateCommentsCache();
	else if (name == "notifications")
		UpdateSlaveNotifications();
	else if (name == "check_interval") {
		ConfigItem::Ptr item = ConfigItem::GetObject("Service", GetName());

		/* update the next check timestamp if we're the owner of this service */
		if (item)
			UpdateNextCheck();
	}
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
