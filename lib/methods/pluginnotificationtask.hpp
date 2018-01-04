/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef PLUGINNOTIFICATIONTASK_H
#define PLUGINNOTIFICATIONTASK_H

#include "methods/i2-methods.hpp"
#include "icinga/notification.hpp"
#include "icinga/service.hpp"
#include "base/process.hpp"

namespace icinga
{

/**
 * Implements sending notifications based on external plugins.
 *
 * @ingroup methods
 */
class PluginNotificationTask
{
public:
	static void ScriptFunc(const Notification::Ptr& notification,
		const User::Ptr& user, const CheckResult::Ptr& cr, int itype,
		const String& author, const String& comment,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	PluginNotificationTask();

	static void ProcessFinishedHandler(const Checkable::Ptr& checkable,
		const Value& commandLine, const ProcessResult& pr);
};

}

#endif /* PLUGINNOTIFICATIONTASK_H */
