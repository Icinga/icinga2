/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
		const User::Ptr& user, const CheckResult::Ptr& cr, const NotificationResult::Ptr& nr,
		int itype, const String& author, const String& comment,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	PluginNotificationTask();

	static void ProcessFinishedHandler(const Checkable::Ptr& checkable,
		const Notification::Ptr& notification, const NotificationResult::Ptr& nr,
		const Value& commandLine, const ProcessResult& pr);
};

}

#endif /* PLUGINNOTIFICATIONTASK_H */
