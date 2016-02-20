/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "icinga/notificationcommand.hpp"
#include "icinga/notificationcommand.tcpp"

using namespace icinga;

REGISTER_TYPE(NotificationCommand);

Dictionary::Ptr NotificationCommand::Execute(const Notification::Ptr& notification,
    const User::Ptr& user, const CheckResult::Ptr& cr, const NotificationType& type,
    const String& author, const String& comment, const Dictionary::Ptr& resolvedMacros,
    bool useResolvedMacros)
{
	std::vector<Value> arguments;
	arguments.push_back(notification);
	arguments.push_back(user);
	arguments.push_back(cr);
	arguments.push_back(type);
	arguments.push_back(author);
	arguments.push_back(comment);
	arguments.push_back(resolvedMacros);
	arguments.push_back(useResolvedMacros);
	return GetExecute()->Invoke(arguments);
}
