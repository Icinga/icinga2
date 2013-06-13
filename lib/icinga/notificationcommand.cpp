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

#include "icinga/notificationcommand.h"
#include "base/dynamictype.h"

using namespace icinga;

REGISTER_TYPE(NotificationCommand);

/**
 * Constructor for the NotificationCommand class.
 *
 * @param serializedUpdate A serialized dictionary containing attributes.
 */
NotificationCommand::NotificationCommand(const Dictionary::Ptr& serializedUpdate)
	: Command(serializedUpdate)
{ }

NotificationCommand::Ptr NotificationCommand::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("NotificationCommand", name);

	return dynamic_pointer_cast<NotificationCommand>(configObject);
}

Dictionary::Ptr NotificationCommand::Execute(const Notification::Ptr& notification,
    const User::Ptr& user, const Dictionary::Ptr& cr, const NotificationType& type)
{
	std::vector<Value> arguments;
	arguments.push_back(notification);
	arguments.push_back(user);
	arguments.push_back(cr);
	arguments.push_back(type);
	return InvokeMethod("execute", arguments);
}
