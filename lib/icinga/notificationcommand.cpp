/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/notificationcommand.hpp"
#include "icinga/notificationcommand-ti.cpp"

using namespace icinga;

REGISTER_TYPE(NotificationCommand);

Dictionary::Ptr NotificationCommand::Execute(const Notification::Ptr& notification,
	const User::Ptr& user, const CheckResult::Ptr& cr, const NotificationResult::Ptr& nr,
	const NotificationType& type, const String& author, const String& comment,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	return GetExecute()->Invoke({
		notification,
		user,
		cr,
		nr,
		type,
		author,
		comment,
		resolvedMacros,
		useResolvedMacros,
	});
}
