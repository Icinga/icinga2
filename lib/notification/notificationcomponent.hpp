/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NOTIFICATIONCOMPONENT_H
#define NOTIFICATIONCOMPONENT_H

#include "notification/notificationcomponent-ti.hpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"

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

	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

	void StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type);

private:

};

}

#endif /* NOTIFICATIONCOMPONENT_H */
