/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NOTIFICATIONCOMMAND_H
#define NOTIFICATIONCOMMAND_H

#include "icinga/notificationcommand-ti.hpp"
#include "icinga/notification.hpp"

namespace icinga
{

class Notification;

/**
 * A notification command.
 *
 * @ingroup icinga
 */
class NotificationCommand final : public ObjectImpl<NotificationCommand>
{
public:
	DECLARE_OBJECT(NotificationCommand);
	DECLARE_OBJECTNAME(NotificationCommand);

	virtual Dictionary::Ptr Execute(const intrusive_ptr<Notification>& notification,
		const User::Ptr& user, const CheckResult::Ptr& cr, const NotificationResult::Ptr& nr,
		const NotificationType& type, const String& author, const String& comment,
		const Dictionary::Ptr& resolvedMacros = nullptr,
		bool useResolvedMacros = false);
};

}

#endif /* NOTIFICATIONCOMMAND_H */
