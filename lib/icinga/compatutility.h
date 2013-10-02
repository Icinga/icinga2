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

#ifndef COMPATUTILITY_H
#define COMPATUTILITY_H

#include "icinga/i2-icinga.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include "base/dictionary.h"
#include "base/dynamicobject.h"
#include <vector>

namespace icinga
{

/**
 * @ingroup icinga
 */
enum CompatObjectType
{
	CompatTypeService,
	CompatTypeHost
};

/**
 * Compatibility utility functions.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API CompatUtility
{
public:
	static Dictionary::Ptr GetHostConfigAttributes(const Host::Ptr& host);

	static Dictionary::Ptr GetServiceStatusAttributes(const Service::Ptr& service, CompatObjectType type);
	static Dictionary::Ptr GetServiceConfigAttributes(const Service::Ptr& service);

	static Dictionary::Ptr GetCommandConfigAttributes(const Command::Ptr& command);

	static Dictionary::Ptr GetCustomVariableConfig(const DynamicObject::Ptr& object);

	static std::set<User::Ptr> GetServiceNotificationUsers(const Service::Ptr& service);
	static std::set<UserGroup::Ptr> GetServiceNotificationUserGroups(const Service::Ptr& service);

	static Dictionary::Ptr GetCheckResultOutput(const Dictionary::Ptr& cr);
	static String GetCheckResultPerfdata(const Dictionary::Ptr& cr);

	static int MapNotificationReasonType(NotificationType type);
	static int MapExternalCommandType(const String& name);

	static String EscapeString(const String& str);

private:
	CompatUtility(void);
};

}

#endif /* COMPATUTILITY_H */
