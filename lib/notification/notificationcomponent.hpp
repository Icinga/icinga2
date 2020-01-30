/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NOTIFICATIONCOMPONENT_H
#define NOTIFICATIONCOMPONENT_H

#include "base/configobject.hpp"
#include "base/timer.hpp"
#include "icinga/service.hpp"
#include "notification/notificationcomponent-ti.hpp"

namespace icinga
{

/**
 * @ingroup notification
 */
class NotificationComponent final : public ObjectImpl<NotificationComponent>
{
public:
	DECLARE_OBJECT(NotificationComponent);
	DECLARE_OBJECTNAME(NotificationComponent);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	Timer::Ptr m_NotificationTimer;

	void NotificationTimerHandler();
	void SendNotificationsHandler(const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr& cr, const String& author, const String& text);
};

}

#endif /* NOTIFICATIONCOMPONENT_H */
