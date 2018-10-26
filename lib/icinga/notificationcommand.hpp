/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef NOTIFICATIONCOMMAND_H
#define NOTIFICATIONCOMMAND_H

#include "icinga/notificationcommand.thpp"
#include "icinga/notification.hpp"

namespace icinga
{

class Notification;

/**
 * A notification command.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API NotificationCommand : public ObjectImpl<NotificationCommand>
{
public:
	DECLARE_OBJECT(NotificationCommand);
	DECLARE_OBJECTNAME(NotificationCommand);

	virtual Dictionary::Ptr Execute(const intrusive_ptr<Notification>& notification,
		const User::Ptr& user, const CheckResult::Ptr& cr, const NotificationType& type,
	    const String& author, const String& comment,
	    const Dictionary::Ptr& resolvedMacros = nullptr,
	    bool useResolvedMacros = false);
};

}

#endif /* NOTIFICATIONCOMMAND_H */
