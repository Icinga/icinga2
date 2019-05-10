/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "notification/notificationcomponent.hpp"
#include "notification/notificationcomponent-ti.cpp"
#include "base/perfdatavalue.hpp"
#include "base/statsfunction.hpp"


using namespace icinga;

REGISTER_TYPE(NotificationComponent);
REGISTER_STATSFUNCTION(NotificationComponent, &NotificationComponent::StatsFunc);

void NotificationComponent::OnConfigLoaded()
{
	ConfigObject::OnActiveChanged.connect(std::bind(&NotificationComponent::ObjectHandler, this, _1));
	ConfigObject::OnPausedChanged.connect(std::bind(&NotificationComponent::ObjectHandler, this, _1));

	Checkable::OnStateChange.connect(std::bind(&NotificationComponent::StateChangeHandler, this, _1, _2, _3));
	Checkable::OnFlappingChanged.connect(std::bind(&NotificationComponent::FlappingChangedHandler, this, _1));

	Checkable::OnAcknowledgementSet.connect(std::bind(&NotificationComponent::SetAcknowledgementHandler, this, _1, _2, _3));

	Downtime::OnDowntimeTriggered.connect(std::bind(&NotificationComponent::TriggerDowntimeHandler, this, _1));
	Downtime::OnDowntimeRemoved.connect(std::bind(&NotificationComponent::RemoveDowntimeHandler, this, _1));

	Notification::OnNextNotificationChanged.connect(std::bind(&NotificationComponent::NextNotificationChangedHandler, this, _1));

}

void NotificationComponent::Start(bool runtimeCreated)
{
	ObjectImpl<NotificationComponent>::Start(runtimeCreated);

	Log(LogInformation, "NotificationComponent")
		<< "'" << GetName() << "' started.";

	m_Thread = std::thread(std::bind(&NotificationComponent::NotificationThreadProc, this));
}

void NotificationComponent::Stop(bool runtimeRemoved)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_Stopped = true;
		m_CV.notify_all();
	}

	while (GetPendingNotifications() > 0) {
		Utility::Sleep(0.1);
		Log(LogCritical, "DEBUG", "waiting for notifications...");
	}

	m_Thread.join();

	Log(LogInformation, "NotificationComponent")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<NotificationComponent>::Stop(runtimeRemoved);
}

void NotificationComponent::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData nodes;

	for (const NotificationComponent::Ptr& notifier : ConfigType::GetObjectsByType<NotificationComponent>()) {
		unsigned long idle = notifier->GetIdleNotifications();
		unsigned long pending = notifier->GetPendingNotifications();

		nodes.emplace_back(notifier->GetName(), new Dictionary({
			{ "idle", idle },
			{ "pending", pending }
		}));

		String perfdata_prefix = "notificationcomponent_" + notifier->GetName() + "_";
		perfdata->Add(new PerfdataValue(perfdata_prefix + "idle", Convert::ToDouble(idle)));
		perfdata->Add(new PerfdataValue(perfdata_prefix + "pending", Convert::ToDouble(pending)));
	}

	status->Set("notificationcomponent", new Dictionary(std::move(nodes)));
}

// It's dumb to go through this, but we need to support SetNextNotification
void NotificationComponent::NextNotificationChangedHandlerHelper(const Notification::Ptr& notification)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	/* remove and re-insert the object from the set in order to force an index update */
	typedef boost::multi_index::nth_index<NotificationSet, 0>::type MessageView;
	MessageView& idx = boost::get<0>(m_IdleNotifications);

	auto it = idx.find(notification);

	if (it != idx.end())
		idx.erase(notification);

	NotificationScheduleInfo nsi = GetNotificationScheduleInfo(notification);
	idx.insert(nsi);

	m_CV.notify_all();
}


//PROBLEM: ThreadProc sets next notification, triggers this aaaaand deadlock
void NotificationComponent::NextNotificationChangedHandler(const Notification::Ptr& notification)
{
	Log(LogCritical, "DEBUG")
		<< "GOT IT " << notification->GetName() << " next: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", notification->GetNextNotification());

	// Need to somehow ignore the set next notification things set by reminder notifications?!
	Utility::QueueAsyncCallback(std::bind(&NotificationComponent::NextNotificationChangedHandlerHelper, NotificationComponent::Ptr(this), notification));

}

void NotificationComponent::StateChangeHelper(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	// Recovery
	if (cr->GetState() == 0) {
		Log(LogCritical, "DEBUG")
			<< checkable->GetName() << " recovered. Checking for notifications";
		for (const Notification::Ptr notification : checkable->GetNotifications()) {
			if (!notification->IsPaused() && HardStateNotificationCheck(checkable))
				notification->BeginExecuteNotification(NotificationRecovery, cr, false);
			else if (notification->IsPaused())
				Log(LogCritical, "DEBUG") << notification->GetName() << " is paused";

			typedef boost::multi_index::nth_index<NotificationSet, 0>::type MessageView;
			MessageView& idx = boost::get<0>(m_IdleNotifications);

			auto it = idx.find(notification);

			if (it == idx.end())
				continue;

			idx.erase(notification);
		}
	//Problem
	} else {
		Log(LogCritical, "DEBUG")
				<< checkable->GetName() << " had a hard state change. Checking for notifications";
		for (const Notification::Ptr notification : checkable->GetNotifications()) {
			typedef boost::multi_index::nth_index<NotificationSet, 0>::type MessageView;
			MessageView& idx = boost::get<0>(m_IdleNotifications);

			auto it = idx.find(notification);

			if (it != idx.end())
				continue;

			if (notification->IsPaused())
				Log(LogCritical, "DEBUG") << notification->GetName() << " is paused";
			else if (!HardStateNotificationCheck(checkable))
				Log(LogCritical, "DEBUG") << notification->GetName() << " failed the test";
			else
				notification->BeginExecuteNotification(NotificationProblem, cr, false);

//			m_IdleNotifications.insert(GetNotificationScheduleInfo(notification));
		}
	}
	m_CV.notify_all();
}

void NotificationComponent::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
	// Need to know if this was a recovery (state = ok?)
	if (type != StateTypeHard)
		return;

	Utility::QueueAsyncCallback(std::bind(&NotificationComponent::StateChangeHelper, this, checkable, cr));
}

void NotificationComponent::FlappingChangeHelper(const Checkable::Ptr& checkable, NotificationType type)
{
	for (const Notification::Ptr notification : checkable->GetNotifications()) {
		Log(LogCritical, "DEBUG")
				<< "Checkable " << checkable->GetName() << " his flapping and wants to check Notification "
				<< notification->GetName();

		SendMessageHelper(notification, type, false);
	}
}

void NotificationComponent::FlappingChangedHandler(const Checkable::Ptr& checkable)
{
	NotificationType ntype = checkable->IsFlapping() ? NotificationFlappingStart : NotificationFlappingEnd;
	Log(LogCritical, "DEBUG") << checkable->GetName() << " is flapping!";

	Utility::QueueAsyncCallback(std::bind(&NotificationComponent::FlappingChangeHelper, this, checkable, ntype));
}

void NotificationComponent::SetAcknowledgementHelper(const Checkable::Ptr& checkable)
{
	for (const Notification::Ptr notification : checkable->GetNotifications()) {
		SendMessageHelper(notification, NotificationAcknowledgement, false);
	}
}

void NotificationComponent::SetAcknowledgementHandler(const Checkable::Ptr& checkable, const String& author, const String& text)
{
	Log(LogCritical, "DEBUG")
			<< "Checkable " << checkable->GetName() << " has been acknowledged.";

	Utility::QueueAsyncCallback(std::bind(&NotificationComponent::SetAcknowledgementHelper, this, checkable));
}

void NotificationComponent::TriggerDowntimeHelper(const Downtime::Ptr& downtime)
{
	Checkable::Ptr checkable = downtime->GetCheckable();

	for (const Notification::Ptr notification : checkable->GetNotifications()) {
		SendMessageHelper(notification, NotificationDowntimeStart, false);
	}
}

void NotificationComponent::TriggerDowntimeHandler(const Downtime::Ptr& downtime)
{
	Log(LogCritical, "DEBUG")
			<< "Downtime " << downtime->GetName() << " has been triggered.";

	Utility::QueueAsyncCallback(std::bind(&NotificationComponent::TriggerDowntimeHelper, this, downtime));

}

void NotificationComponent::RemoveDowntimeHandler(const Downtime::Ptr& downtime)
{
	Log(LogCritical, "DEBUG")
		<< "Downtime " << downtime->GetName() << " has been removed.";
}

void NotificationComponent::NotificationThreadProc()
{
	Utility::SetThreadName("Notification Scheduler");

	boost::mutex::scoped_lock lock(m_Mutex);

	for (;;) {
		typedef boost::multi_index::nth_index<NotificationSet, 1>::type NotificationTimeView;
		NotificationTimeView& idx = boost::get<1>(m_IdleNotifications);
		while (idx.begin() == idx.end() && !m_Stopped)
			m_CV.wait(lock);

		if (m_Stopped)
			break;

		auto it = idx.begin();
		NotificationScheduleInfo nsi = *it;

		double wait = nsi.NextMessage - Utility::GetTime();

		Log(LogCritical, "DEBUG") << "Waiting on" << nsi.Object->GetName() << " for "
		<< (long(wait * 1000 * 1000)) << " seconds";
		if (wait > 0) {
			m_CV.timed_wait(lock, boost::posix_time::milliseconds(long(wait * 1000)));

			continue;
		}

		Notification::Ptr notification = nsi.Object;
		m_IdleNotifications.erase(notification);

		// Check for execution needed

		nsi = GetNotificationScheduleInfo(notification);

		Log(LogCritical, "NotificationComponent")
				<< "Scheduling info for notification '" << notification->GetName() << "' ("
				<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", notification->GetNextNotification()) << "): Object '"
				<< nsi.Object->GetName() << "', Next Message: "
				<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", nsi.NextMessage) << "(" << nsi.NextMessage << ").";

		m_PendingNotifications.insert(nsi);

		lock.unlock();
		Log(LogCritical, "DEBUG", "Please execute");
		Utility::QueueAsyncCallback(std::bind(&NotificationComponent::SendReminderNotification, NotificationComponent::Ptr(this), notification));
		Log(LogCritical, "DEBUG")
			<< "Executed??? Next one at " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", notification->GetNextNotification());
		lock.lock();
	}
}

bool NotificationComponent::HardStateNotificationCheck(const Checkable::Ptr& checkable)
{
	bool send_notification = true;

	// Don't send in these cases
	if (!checkable->IsReachable(DependencyNotification) || checkable->IsInDowntime()
	|| checkable->IsAcknowledged() || checkable->IsFlapping()) {
		Log(LogCritical, "DEBUG")
			<< "Not Sending because " << checkable->GetName() << " is "
			<< (!checkable->IsReachable(DependencyNotification) ? "not reachable" :
			checkable->IsInDowntime() ? "in downtime" : checkable->IsAcknowledged() ? "acknowledged":  "flapping");
		return false;
	}

	// We know checkable is in a Hard State, Second case is Recovery
	if ((checkable->GetLastStateType() == StateTypeSoft) ||
						(checkable->GetLastStateType() == StateTypeHard
						&& checkable->GetLastStateRaw() != ServiceOK
						&& checkable->GetStateRaw() == ServiceOK)) {
		send_notification = true;
		Log(LogCritical, "DEBUG")
			<< "Sending because soft -> hard | recovery: " << checkable->GetName();
	}

	/* Or if the checkable is volatile and in a HARD state. */
	if (checkable->GetVolatile()) {
		send_notification = true;
		Log(LogCritical, "DEBUG")
			<< "Sending because volatile & hard state: " << checkable->GetName();
	}

	if (checkable->GetLastStateRaw() == ServiceOK && checkable->GetLastStateType() == StateTypeSoft) {
		send_notification = false; /* Don't send notifications for SOFT-OK -> HARD-OK. */
		Log(LogCritical, "DEBUG")
			<< "Not sending becuase soft-ok -> hard-ok: " << checkable->GetName();
	}

	if (checkable->GetVolatile() && checkable->GetLastStateRaw() == ServiceOK && checkable->GetStateRaw() == ServiceOK) {
		send_notification = false; /* Don't send notifications for volatile OK -> OK changes. */
		Log(LogCritical, "DEBUG")
			<< "Not sending because volatile & ok -> ok: " << checkable->GetName();
	}

	return send_notification;
}

void NotificationComponent::SendReminderNotification(const Notification::Ptr& notification)
{
	auto it = m_PendingNotifications.find(notification);
	if (it != m_PendingNotifications.end()) {
		m_PendingNotifications.erase(it);
	}

	if (notification->IsPaused()) {
		Log(LogCritical, "DEBUG")
				<< notification->GetName() << " is paused";
		notification->SetNextNotification(notification->GetNextNotification() + notification->GetInterval());
		return;
	}

	Checkable::Ptr checkable = notification->GetCheckable();
	if (HardStateNotificationCheck(checkable))
		notification->BeginExecuteNotification(NotificationProblem, checkable->GetLastCheckResult(), false, true);
	else
		notification->SetNextNotification(notification->GetNextNotification() + notification->GetInterval());

}

void NotificationComponent::SendMessageHelper(const Notification::Ptr& notification, NotificationType type, bool reminder)
{
	if (notification->IsPaused()) {
		Log(LogCritical, "DEBUG")
			<< notification->GetName() << " is paused";
		boost::mutex::scoped_lock lock(m_Mutex);
		auto it = m_PendingNotifications.find(notification);
		if (it != m_PendingNotifications.end()) {
			m_PendingNotifications.erase(it);

			if (type == NotificationProblem) {
				notification->SetNextNotification(notification->GetNextNotification() + notification->GetInterval());
				m_IdleNotifications.insert(GetNotificationScheduleInfo(notification));
			}
		}
		m_CV.notify_all();

		return;
	}

	Log(LogCritical, "DEBUG") << "Executing Next Message at " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", notification->GetNextNotification());

	if (type == NotificationProblem) {
		if (HardStateNotificationCheck(notification->GetCheckable()))
			notification->BeginExecuteNotification(type, notification->GetCheckable()->GetLastCheckResult(), false, reminder);
		else
			notification->SetNextNotification(notification->GetNextNotification() + notification->GetNextNotification());

	} else if (type == NotificationRecovery) {
		if (HardStateNotificationCheck(notification->GetCheckable()))
			notification->BeginExecuteNotification(type, notification->GetCheckable()->GetLastCheckResult(), false, reminder);
	} else {
		notification->BeginExecuteNotification(type, notification->GetCheckable()->GetLastCheckResult(), false, reminder);
	}


	boost::mutex::scoped_lock lock(m_Mutex);
	auto it = m_PendingNotifications.find(notification);

	if (it != m_PendingNotifications.end()) {
		m_PendingNotifications.erase(it);

		if (notification->IsActive() && type == NotificationProblem)
			m_IdleNotifications.insert(GetNotificationScheduleInfo(notification));

		m_CV.notify_all();
	}
}

void NotificationComponent::ObjectHandler(const ConfigObject::Ptr& object)
{
	Notification::Ptr notification = dynamic_pointer_cast<Notification>(object);

	if (!notification)
		return;

	Zone::Ptr zone = Zone::GetByName(notification->GetZoneName());
	bool same_zone = (!zone || Zone::GetLocalZone() == zone);

	Checkable::Ptr checkable = notification->GetCheckable();

	bool reachable = checkable->IsReachable(DependencyNotification);


	{
		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(checkable);

		ObjectLock olock(checkable);

		if (checkable->GetStateType() == StateTypeSoft)
			return;

		if ((service && service->GetState() == ServiceOK) || (!service && host->GetState() == HostUp))
			return;

		if (!reachable || checkable->IsInDowntime() || checkable->IsAcknowledged() || checkable->IsFlapping())
			return;
	}

	{
		boost::mutex::scoped_lock lock(m_Mutex);
		if (object->IsActive() && !object->IsPaused() && same_zone) {
			if (m_PendingNotifications.find(notification) != m_PendingNotifications.end())
				return;

			Log(LogCritical, "DEBUG") << notification->GetName() << " at " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", notification->GetNextNotification());
			m_IdleNotifications.insert(GetNotificationScheduleInfo(notification));
		} else {
			m_IdleNotifications.erase(notification);
			m_PendingNotifications.erase(notification);
		}

		m_CV.notify_all();
	}
}

NotificationScheduleInfo NotificationComponent::GetNotificationScheduleInfo(const Notification::Ptr& notification)
{
	NotificationScheduleInfo nsi;
	nsi.Object = notification;
	nsi.NextMessage = notification->GetNextNotification();
	return nsi;
}

unsigned long NotificationComponent::GetIdleNotifications()
{
	boost::mutex::scoped_lock lock(m_Mutex);

	return m_IdleNotifications.size();
}

unsigned long NotificationComponent::GetPendingNotifications()
{
	boost::mutex::scoped_lock lock(m_Mutex);

	return m_PendingNotifications.size();
}
