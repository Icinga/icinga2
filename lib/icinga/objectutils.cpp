/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

REGISTER_SCRIPTFUNCTION(get_host, &Host::GetByName);
REGISTER_SCRIPTFUNCTION(get_service, &ObjectUtils::GetService);
REGISTER_SCRIPTFUNCTION(get_user, &User::GetByName);
REGISTER_SCRIPTFUNCTION(get_check_command, &CheckCommand::GetByName);
REGISTER_SCRIPTFUNCTION(get_event_command, &EventCommand::GetByName);
REGISTER_SCRIPTFUNCTION(get_notification_command, &NotificationCommand::GetByName);
REGISTER_SCRIPTFUNCTION(get_host_group, &HostGroup::GetByName);
REGISTER_SCRIPTFUNCTION(get_service_group, &ServiceGroup::GetByName);
REGISTER_SCRIPTFUNCTION(get_user_group, &UserGroup::GetByName);

Service::Ptr ObjectUtils::GetService(const String& host, const String& name)
{
	Host::Ptr host_obj = Host::GetByName(host);

	if (!host_obj)
		return Service::Ptr();

	return host_obj->GetServiceByShortName(name);
}

