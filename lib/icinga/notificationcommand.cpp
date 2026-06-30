// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/notificationcommand.hpp"
#include "icinga/notificationcommand-ti.cpp"

using namespace icinga;

REGISTER_TYPE(NotificationCommand);

thread_local NotificationCommand::Ptr NotificationCommand::ExecuteOverride;

Dictionary::Ptr NotificationCommand::Execute(const Notification::Ptr& notification,
	const User::Ptr& user, const CheckResult::Ptr& cr, const NotificationType& type,
	const String& author, const String& comment, const Dictionary::Ptr& resolvedMacros,
	bool useResolvedMacros)
{
	return GetExecute()->Invoke({
		notification,
		user,
		cr,
		type,
		author,
		comment,
		resolvedMacros,
		useResolvedMacros,
	});
}
