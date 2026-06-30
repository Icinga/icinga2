// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/objectutils.hpp"
#include "icinga/host.hpp"
#include "icinga/user.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/servicegroup.hpp"
#include "icinga/usergroup.hpp"

using namespace icinga;

REGISTER_FUNCTION(Icinga, get_host, &Host::GetByName, "name");
REGISTER_FUNCTION(Icinga, get_service, &ObjectUtils::GetService, "host:name");
REGISTER_FUNCTION(Icinga, get_services, &ObjectUtils::GetServices, "host");
REGISTER_FUNCTION(Icinga, get_user, &User::GetByName, "name");
REGISTER_FUNCTION(Icinga, get_check_command, &CheckCommand::GetByName, "name");
REGISTER_FUNCTION(Icinga, get_event_command, &EventCommand::GetByName, "name");
REGISTER_FUNCTION(Icinga, get_notification_command, &NotificationCommand::GetByName, "name");
REGISTER_FUNCTION(Icinga, get_host_group, &HostGroup::GetByName, "name");
REGISTER_FUNCTION(Icinga, get_service_group, &ServiceGroup::GetByName, "name");
REGISTER_FUNCTION(Icinga, get_user_group, &UserGroup::GetByName, "name");
REGISTER_FUNCTION(Icinga, get_time_period, &TimePeriod::GetByName, "name");

Service::Ptr ObjectUtils::GetService(const Value& host, const String& name)
{
	Host::Ptr hostObj;

	if (host.IsObjectType<Host>())
		hostObj = host;
	else
		hostObj = Host::GetByName(host);

	if (!hostObj)
		return nullptr;

	return hostObj->GetServiceByShortName(name);
}

Array::Ptr ObjectUtils::GetServices(const Value& host)
{
	Host::Ptr hostObj;

	if (host.IsObjectType<Host>())
		hostObj = host;
	else
		hostObj = Host::GetByName(host);

	if (!hostObj)
		return nullptr;

	return Array::FromVector(hostObj->GetServices());
}
