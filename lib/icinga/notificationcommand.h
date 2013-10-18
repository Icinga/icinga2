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

#ifndef NOTIFICATIONCOMMAND_H
#define NOTIFICATIONCOMMAND_H

#include "icinga/command.h"
#include "icinga/notification.h"

namespace icinga
{

class Notification;

/**
 * A notification command.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API NotificationCommand : public Command
{
public:
	DECLARE_PTR_TYPEDEFS(NotificationCommand);
	DECLARE_TYPENAME(NotificationCommand);

	virtual Dictionary::Ptr Execute(const shared_ptr<Notification>& notification,
	    const User::Ptr& user, const Dictionary::Ptr& cr, const NotificationType& type,
	    const String& author, const String& comment);
};

}

#endif /* NOTIFICATIONCOMMAND_H */
