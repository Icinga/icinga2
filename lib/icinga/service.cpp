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

#include "i2-icinga.h"

using namespace icinga;

REGISTER_TYPE(Service, NULL);

Service::Service(const Dictionary::Ptr& serializedObject)
	: DynamicObject(serializedObject)
{

	RegisterAttribute("display_name", Attribute_Config, &m_DisplayName);
	RegisterAttribute("macros", Attribute_Config, &m_Macros);
	RegisterAttribute("hostdependencies", Attribute_Config, &m_HostDependencies);
	RegisterAttribute("servicedependencies", Attribute_Config, &m_ServiceDependencies);
	RegisterAttribute("servicegroups", Attribute_Config, &m_ServiceGroups);

	RegisterAttribute("check_command", Attribute_Config, &m_CheckCommand);
	RegisterAttribute("max_check_attempts", Attribute_Config, &m_MaxCheckAttempts);
	RegisterAttribute("check_interval", Attribute_Config, &m_CheckInterval);
	RegisterAttribute("retry_interval", Attribute_Config, &m_RetryInterval);
	RegisterAttribute("checkers", Attribute_Config, &m_Checkers);

	RegisterAttribute("next_check", Attribute_Replicated, &m_NextCheck);
	RegisterAttribute("current_checker", Attribute_Replicated, &m_CurrentChecker);
	RegisterAttribute("check_attempt", Attribute_Replicated, &m_CheckAttempt);
	RegisterAttribute("state", Attribute_Replicated, &m_State);
	RegisterAttribute("state_type", Attribute_Replicated, &m_StateType);
	RegisterAttribute("last_result", Attribute_Replicated, &m_LastResult);
	RegisterAttribute("last_state_change", Attribute_Replicated, &m_LastStateChange);
	RegisterAttribute("last_hard_state_change", Attribute_Replicated, &m_LastHardStateChange);
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
	RegisterAttribute("last_notification", Attribute_Replicated, &m_LastNotification);
	RegisterAttribute("notification_interval", Attribute_Config, &m_NotificationInterval);

	SetSchedulingOffset(rand());
}

Service::~Service(void)
{
	ServiceGroup::RefreshMembersCache();
	Host::RefreshServicesCache();
	Service::RefreshDowntimesCache();
	Service::RefreshCommentsCache();
}

void Service::OnRegistrationCompleted(void)
{
	DynamicObject::OnRegistrationCompleted();

	ServiceGroup::RefreshMembersCache();
	Host::RefreshServicesCache();
}

String Service::GetDisplayName(void) const
{
	if (m_DisplayName.IsEmpty())
		return GetShortName();
	else
		return m_DisplayName;
}

/**
 * @threadsafety Always.
 */
bool Service::Exists(const String& name)
{
	return (DynamicObject::GetObject("Service", name));
}

/**
 * @threadsafety Always.
 */
Service::Ptr Service::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Service", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(invalid_argument("Service '" + name + "' does not exist."));

	return dynamic_pointer_cast<Service>(configObject);
}

/**
 * @threadsafety Always.
 */
Service::Ptr Service::GetByNamePair(const String& hostName, const String& serviceName)
{
	if (!hostName.IsEmpty()) {
		Host::Ptr host = Host::GetByName(hostName);
		return Host::GetServiceByShortName(host, serviceName);
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

Dictionary::Ptr Service::GetHostDependencies(void) const
{
	return m_HostDependencies;
}

Dictionary::Ptr Service::GetServiceDependencies(void) const
{
	return m_ServiceDependencies;
}

Dictionary::Ptr Service::GetGroups(void) const
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

bool Service::IsReachable(const Service::Ptr& self)
{
	BOOST_FOREACH(const Service::Ptr& service, Service::GetParentServices(self)) {
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

	BOOST_FOREACH(const Host::Ptr& host, Service::GetParentHosts(self)) {
		Service::Ptr hc = Host::GetHostCheckService(host);
		ObjectLock olock(hc);

		/* ignore hosts that are up */
		if (hc && hc->GetState() == StateOK)
			continue;

		return false;
	}

	return true;
}

AcknowledgementType Service::GetAcknowledgement(void)
{
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

void Service::AcknowledgeProblem(AcknowledgementType type, double expiry)
{
	SetAcknowledgement(type);
	SetAcknowledgementExpiry(expiry);

	RequestNotifications(NotificationAcknowledgement);
}

void Service::ClearAcknowledgement(void)
{
	SetAcknowledgement(AcknowledgementNone);
	SetAcknowledgementExpiry(0);
}

void Service::OnAttributeChanged(const String& name, const Value& oldValue)
{
	if (name == "current_checker")
		OnCheckerChanged(GetSelf(), oldValue);
	else if (name == "next_check")
		OnNextCheckChanged(GetSelf(), oldValue);
	else if (name == "servicegroups")
		ServiceGroup::RefreshMembersCache();
	else if (name == "host_name" || name == "short_name") {
		Host::RefreshServicesCache();

		UpdateSlaveNotifications(GetSelf());
	} else if (name == "downtimes")
		Service::RefreshDowntimesCache();
	else if (name == "comments")
		Service::RefreshCommentsCache();
	else if (name == "notifications")
		UpdateSlaveNotifications(GetSelf());
	else if (name == "check_interval") {
		ObjectLock(this);
		ConfigItem::Ptr item = ConfigItem::GetObject("Service", GetName());

		/* update the next check timestamp if we're the owner of this service */
		if (item && !IsAbstract())
			UpdateNextCheck();
	}
}

set<Host::Ptr> Service::GetParentHosts(const Service::Ptr& self)
{
	set<Host::Ptr> parents;

	Dictionary::Ptr dependencies;

	{
		ObjectLock olock(self);

		/* The service's host is implicitly a parent. */
		parents.insert(self->GetHost());

		dependencies = self->GetHostDependencies();
	}


	if (dependencies) {
		String key;
		BOOST_FOREACH(tie(key, tuples::ignore), dependencies) {
			parents.insert(Host::GetByName(key));
		}
	}

	return parents;
}

set<Service::Ptr> Service::GetParentServices(const Service::Ptr& self)
{
	set<Service::Ptr> parents;

	Dictionary::Ptr dependencies;
	Host::Ptr host;
	String service_name;

	{
		ObjectLock olock(self);
		dependencies = self->GetServiceDependencies();
		host = self->GetHost();
		service_name = self->GetName();
	}

	if (dependencies) {
		String key;
		Value value;
		BOOST_FOREACH(tie(key, value), dependencies) {
			Service::Ptr service = Host::GetServiceByShortName(host, value);
			ObjectLock olock(service);

			if (service->GetName() == service_name)
				continue;

			parents.insert(service);
		}
	}

	return parents;
}

Dictionary::Ptr Service::CalculateDynamicMacros(const Service::Ptr& self)
{
	Dictionary::Ptr macros = boost::make_shared<Dictionary>();

	Dictionary::Ptr cr;

	{
		ObjectLock olock(self);
		macros->Set("SERVICEDESC", self->GetShortName());
		macros->Set("SERVICEDISPLAYNAME", self->GetDisplayName());
		macros->Set("SERVICESTATE", StateToString(self->GetState()));
		macros->Set("SERVICESTATEID", self->GetState());
		macros->Set("SERVICESTATETYPE", StateTypeToString(self->GetStateType()));
		macros->Set("SERVICEATTEMPT", self->GetCurrentCheckAttempt());
		macros->Set("MAXSERVICEATTEMPT", self->GetMaxCheckAttempts());

		cr = self->GetLastCheckResult();
	}

	if (cr) {
		macros->Set("SERVICELATENCY", Service::CalculateLatency(cr));
		macros->Set("SERVICEEXECUTIONTIME", Service::CalculateExecutionTime(cr));

		ObjectLock olock(cr);

		macros->Set("SERVICEOUTPUT", cr->Get("output"));
		macros->Set("SERVICEPERFDATA", cr->Get("performance_data_raw"));
	}

	macros->Seal();

	return macros;
}
