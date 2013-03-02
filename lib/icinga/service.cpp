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

REGISTER_TYPE(Service);

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
	ServiceGroup::InvalidateMembersCache();
	Host::InvalidateServicesCache();
	Service::InvalidateDowntimesCache();
	Service::InvalidateCommentsCache();
}

/**
 * @threadsafety Always.
 */
void Service::OnRegistrationCompleted(void)
{
	assert(!OwnsLock());

	DynamicObject::OnRegistrationCompleted();

	InvalidateNotificationsCache();
}

/**
 * @threadsafety Always.
 */
String Service::GetDisplayName(void) const
{
	ObjectLock olock(this);

	if (m_DisplayName.IsEmpty())
		return GetShortName();
	else
		return m_DisplayName;
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

		if (!host)
			return Service::Ptr();

		return host->GetServiceByShortName(serviceName);
	} else {
		return Service::GetByName(serviceName);
	}
}

/**
 * @threadsafety Always.
 */
Host::Ptr Service::GetHost(void) const
{
	ObjectLock olock(this);

	return Host::GetByName(m_HostName);
}

/**
 * @threadsafety Always.
 */
Dictionary::Ptr Service::GetMacros(void) const
{
	ObjectLock olock(this);

	return m_Macros;
}

/**
 * @threadsafety Always.
 */
Dictionary::Ptr Service::GetHostDependencies(void) const
{
	ObjectLock olock(this);

	return m_HostDependencies;
}

/**
 * @threadsafety Always.
 */
Dictionary::Ptr Service::GetServiceDependencies(void) const
{
	ObjectLock olock(this);

	return m_ServiceDependencies;
}

/**
 * @threadsafety Always.
 */
Dictionary::Ptr Service::GetGroups(void) const
{
	ObjectLock olock(this);

	return m_ServiceGroups;
}

/**
 * @threadsafety Always.
 */
String Service::GetHostName(void) const
{
	ObjectLock olock(this);

	return m_HostName;
}

/**
 * @threadsafety Always.
 */
String Service::GetShortName(void) const
{
	ObjectLock olock(this);

	if (m_ShortName.IsEmpty())
		return GetName();
	else
		return m_ShortName;
}

/**
 * @threadsafety Always.
 */
bool Service::IsReachable(void) const
{
	assert(!OwnsLock());

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

/**
 * @threadsafety Always.
 */
AcknowledgementType Service::GetAcknowledgement(void)
{
	ObjectLock olock(this);

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

/**
 * @threadsafety Always.
 */
void Service::SetAcknowledgement(AcknowledgementType acknowledgement)
{
	ObjectLock olock(this);

	m_Acknowledgement = acknowledgement;
	Touch("acknowledgement");
}

/**
 * @threadsafety Always.
 */
bool Service::IsAcknowledged(void)
{
	return GetAcknowledgement() != AcknowledgementNone;
}

/**
 * @threadsafety Always.
 */
double Service::GetAcknowledgementExpiry(void) const
{
	ObjectLock olock(this);

	if (m_AcknowledgementExpiry.IsEmpty())
		return 0;

	return static_cast<double>(m_AcknowledgementExpiry);
}

/**
 * @threadsafety Always.
 */
void Service::SetAcknowledgementExpiry(double timestamp)
{
	ObjectLock olock(this);

	m_AcknowledgementExpiry = timestamp;
	Touch("acknowledgement_expiry");
}

/**
 * @threadsafety Always.
 */
void Service::AcknowledgeProblem(AcknowledgementType type, double expiry)
{
	ObjectLock olock(this);

	SetAcknowledgement(type);
	SetAcknowledgementExpiry(expiry);

	RequestNotifications(NotificationAcknowledgement);
}

/**
 * @threadsafety Always.
 */
void Service::ClearAcknowledgement(void)
{
	SetAcknowledgement(AcknowledgementNone);
	SetAcknowledgementExpiry(0);
}

/**
 * @threadsafety Always.
 */
void Service::OnAttributeChanged(const String& name, const Value& oldValue)
{
	assert(!OwnsLock());

	Service::Ptr self;
	String service_name;
	bool abstract;

	{
		ObjectLock olock(this);
		self = GetSelf();
		service_name = GetName();
		abstract = IsAbstract();
	}

	if (name == "current_checker")
		OnCheckerChanged(self, oldValue);
	else if (name == "next_check")
		OnNextCheckChanged(self, oldValue);
	else if (name == "servicegroups")
		ServiceGroup::InvalidateMembersCache();
	else if (name == "host_name" || name == "short_name") {
		Host::InvalidateServicesCache();

		UpdateSlaveNotifications(self);
	} else if (name == "downtimes")
		Service::InvalidateDowntimesCache();
	else if (name == "comments")
		Service::InvalidateCommentsCache();
	else if (name == "notifications")
		UpdateSlaveNotifications(self);
	else if (name == "check_interval") {
		ObjectLock olock(this);
		ConfigItem::Ptr item = ConfigItem::GetObject("Service", service_name);

		/* update the next check timestamp if we're the owner of this service */
		if (item && !abstract)
			UpdateNextCheck();
	}
}

/**
 * @threadsafety Always.
 */
set<Host::Ptr> Service::GetParentHosts(void) const
{
	set<Host::Ptr> parents;

	Host::Ptr host = GetHost();

	/* The service's host is implicitly a parent. */
	if (host)
		parents.insert(host);

	Dictionary::Ptr dependencies = GetHostDependencies();

	if (dependencies) {
		ObjectLock olock(dependencies);

		String key;
		BOOST_FOREACH(tie(key, tuples::ignore), dependencies) {
			Host::Ptr host = Host::GetByName(key);

			if (!host)
				continue;

			parents.insert(host);
		}
	}

	return parents;
}

/**
 * @threadsafety Always.
 */
set<Service::Ptr> Service::GetParentServices(void) const
{
	set<Service::Ptr> parents;

	Host::Ptr host = GetHost();
	Dictionary::Ptr dependencies = GetServiceDependencies();

	if (host && dependencies) {
		String key;
		Value value;
		BOOST_FOREACH(tie(key, value), dependencies) {
			Service::Ptr service = host->GetServiceByShortName(value);

			if (!service)
				continue;

			if (service->GetName() == GetName())
				continue;

			parents.insert(service);
		}
	}

	return parents;
}

/**
 * @threadsafety Always.
 */
Dictionary::Ptr Service::CalculateDynamicMacros(void) const
{
	Dictionary::Ptr macros = boost::make_shared<Dictionary>();

	Dictionary::Ptr cr;

	{
		ObjectLock olock(this);
		macros->Set("SERVICEDESC", GetShortName());
		macros->Set("SERVICEDISPLAYNAME", GetDisplayName());
		macros->Set("SERVICESTATE", StateToString(GetState()));
		macros->Set("SERVICESTATEID", GetState());
		macros->Set("SERVICESTATETYPE", StateTypeToString(GetStateType()));
		macros->Set("SERVICEATTEMPT", GetCurrentCheckAttempt());
		macros->Set("MAXSERVICEATTEMPT", GetMaxCheckAttempts());
		macros->Set("SERVICECHECKCOMMAND", "check_i2");

		cr = GetLastCheckResult();
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
