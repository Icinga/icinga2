/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "notification/notificationcomponent.hpp"
#include "notification/notificationcomponent-ti.cpp"


using namespace icinga;

REGISTER_TYPE(NotificationComponent);

/**
 * Starts the component.
 */
void NotificationComponent::Start(bool runtimeCreated)
{
	ObjectImpl<NotificationComponent>::Start(runtimeCreated);

	Log(LogInformation, "NotificationComponent")
		<< "'" << GetName() << "' started.";

	Checkable::OnStateChange.connect(std::bind(&NotificationComponent::StateChangeHandler, this, _1, _2, _3));
}

void NotificationComponent::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "NotificationComponent")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<NotificationComponent>::Stop(runtimeRemoved);
}

void NotificationComponent::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type) {
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	if (type != StateTypeHard) {
		Log(LogCritical, "DEBUG")
				<< "Ignoring soft state change for " << checkable->GetName();
		return;
	}

	for (const Notification::Ptr notifcation : checkable->GetNotifications()) {
		Log(LogCritical, "DEBUG")
			<< "Checkable " << checkable->GetName() << " had a hard change and wants to check Notification "
			<< notifcation->GetName();
			// Check if notification needs to be sent out

		// Queue Renotifications
	}

}

