/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/checkable.hpp"
#include "icinga/checkable-ti.cpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/timer.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(Checkable, Checkable::GetPrototype());
INITIALIZE_ONCE(&Checkable::StaticInitialize);

boost::signals2::signal<void (const Checkable::Ptr&, const String&, const String&, AcknowledgementType, bool, bool, double, double, const MessageOrigin::Ptr&)> Checkable::OnAcknowledgementSet;
boost::signals2::signal<void (const Checkable::Ptr&, const String&, double, const MessageOrigin::Ptr&)> Checkable::OnAcknowledgementCleared;
boost::signals2::signal<void (const Checkable::Ptr&, double)> Checkable::OnFlappingChange;

static Timer::Ptr l_CheckablesFireSuppressedNotifications;
static Timer::Ptr l_CleanDeadlinedExecutions;

thread_local std::function<void(const Value& commandLine, const ProcessResult&)> Checkable::ExecuteCommandProcessFinishedHandler;

void Checkable::StaticInitialize()
{
	/* fixed downtime start */
	Downtime::OnDowntimeStarted.connect([](const Downtime::Ptr& downtime) { Checkable::NotifyFixedDowntimeStart(downtime); });
	/* flexible downtime start */
	Downtime::OnDowntimeTriggered.connect([](const Downtime::Ptr& downtime) { Checkable::NotifyFlexibleDowntimeStart(downtime); });
	/* fixed/flexible downtime end */
	Downtime::OnDowntimeRemoved.connect([](const Downtime::Ptr& downtime) { Checkable::NotifyDowntimeEnd(downtime); });
}

Checkable::Checkable()
{
	SetSchedulingOffset(Utility::Random());
}

void Checkable::OnAllConfigLoaded()
{
	ObjectImpl<Checkable>::OnAllConfigLoaded();

	Endpoint::Ptr endpoint = GetCommandEndpoint();

	if (endpoint) {
		Zone::Ptr checkableZone = static_pointer_cast<Zone>(GetZone());

		if (checkableZone) {
			Zone::Ptr cmdZone = endpoint->GetZone();

			if (cmdZone != checkableZone && cmdZone->GetParent() != checkableZone) {
				BOOST_THROW_EXCEPTION(ValidationError(this, { "command_endpoint" },
					"Command endpoint must be in zone '" + checkableZone->GetName() + "' or in a direct child zone thereof."));
			}
		} else {
			BOOST_THROW_EXCEPTION(ValidationError(this, { "command_endpoint" },
				"Checkable with command endpoint requires a zone. Please check the troubleshooting documentation."));
		}
	}
}

void Checkable::Start(bool runtimeCreated)
{
	double now = Utility::GetTime();

	{
		auto cr (GetLastCheckResult());

		if (GetLastCheckStarted() > (cr ? cr->GetExecutionEnd() : 0.0)) {
			SetNextCheck(GetLastCheckStarted());
		}
	}

	if (GetNextCheck() < now + 60) {
		double delta = std::min(GetCheckInterval(), 60.0);
		delta *= (double)std::rand() / RAND_MAX;
		SetNextCheck(now + delta);
	}

	ObjectImpl<Checkable>::Start(runtimeCreated);

	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, []() {
		l_CheckablesFireSuppressedNotifications = new Timer();
		l_CheckablesFireSuppressedNotifications->SetInterval(5);
		l_CheckablesFireSuppressedNotifications->OnTimerExpired.connect(&Checkable::FireSuppressedNotifications);
		l_CheckablesFireSuppressedNotifications->Start();

		l_CleanDeadlinedExecutions = new Timer();
		l_CleanDeadlinedExecutions->SetInterval(300);
		l_CleanDeadlinedExecutions->OnTimerExpired.connect(&Checkable::CleanDeadlinedExecutions);
		l_CleanDeadlinedExecutions->Start();
	});
}

void Checkable::AddGroup(const String& name)
{
	boost::mutex::scoped_lock lock(m_CheckableMutex);

	Array::Ptr groups;
	auto *host = dynamic_cast<Host *>(this);

	if (host)
		groups = host->GetGroups();
	else
		groups = static_cast<Service *>(this)->GetGroups();

	if (groups && groups->Contains(name))
		return;

	if (!groups)
		groups = new Array();

	groups->Add(name);
}

AcknowledgementType Checkable::GetAcknowledgement()
{
	auto avalue = static_cast<AcknowledgementType>(GetAcknowledgementRaw());

	if (avalue != AcknowledgementNone) {
		double expiry = GetAcknowledgementExpiry();

		if (expiry != 0 && expiry < Utility::GetTime()) {
			avalue = AcknowledgementNone;
			ClearAcknowledgement("");
		}
	}

	return avalue;
}

bool Checkable::IsAcknowledged() const
{
	return const_cast<Checkable *>(this)->GetAcknowledgement() != AcknowledgementNone;
}

void Checkable::AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, bool notify, bool persistent, double changeTime, double expiry, const MessageOrigin::Ptr& origin)
{
	SetAcknowledgementRaw(type);
	SetAcknowledgementExpiry(expiry);

	if (notify && !IsPaused())
		OnNotificationsRequested(this, NotificationAcknowledgement, GetLastCheckResult(), author, comment, nullptr);

	Log(LogInformation, "Checkable")
		<< "Acknowledgement set for checkable '" << GetName() << "'.";

	OnAcknowledgementSet(this, author, comment, type, notify, persistent, changeTime, expiry, origin);

	SetAcknowledgementLastChange(changeTime);
}

void Checkable::ClearAcknowledgement(const String& removedBy, double changeTime, const MessageOrigin::Ptr& origin)
{
	ObjectLock oLock (this);

	bool wasAcked = GetAcknowledgementRaw() != AcknowledgementNone;

	SetAcknowledgementRaw(AcknowledgementNone);
	SetAcknowledgementExpiry(0);

	Log(LogInformation, "Checkable")
		<< "Acknowledgement cleared for checkable '" << GetName() << "'.";

	if (wasAcked) {
		OnAcknowledgementCleared(this, removedBy, changeTime, origin);

		SetAcknowledgementLastChange(changeTime);
	}
}

Endpoint::Ptr Checkable::GetCommandEndpoint() const
{
	return Endpoint::GetByName(GetCommandEndpointRaw());
}

int Checkable::GetSeverity() const
{
	/* overridden in Host/Service class. */
	return 0;
}

bool Checkable::GetProblem() const
{
	auto cr (GetLastCheckResult());

	return cr && !IsStateOK(cr->GetState());
}

bool Checkable::GetHandled() const
{
	return GetProblem() && (IsInDowntime() || IsAcknowledged());
}

Timestamp Checkable::GetNextUpdate() const
{
	auto cr (GetLastCheckResult());
	double interval, latency;

	// TODO: Document this behavior.
	if (cr) {
		interval = GetEnableActiveChecks() && GetProblem() && GetStateType() == StateTypeSoft ? GetRetryInterval() : GetCheckInterval();
		latency = cr->GetExecutionEnd() - cr->GetScheduleStart();
	} else {
		interval = GetCheckInterval();
		latency = 0.0;
	}

	return (GetEnableActiveChecks() ? GetNextCheck() : (cr ? cr->GetExecutionEnd() : Application::GetStartTime()) + interval) + interval + 2 * latency;
}

void Checkable::NotifyFixedDowntimeStart(const Downtime::Ptr& downtime)
{
	if (!downtime->GetFixed())
		return;

	NotifyDowntimeInternal(downtime);
}

void Checkable::NotifyFlexibleDowntimeStart(const Downtime::Ptr& downtime)
{
	if (downtime->GetFixed())
		return;

	NotifyDowntimeInternal(downtime);
}

void Checkable::NotifyDowntimeInternal(const Downtime::Ptr& downtime)
{
	Checkable::Ptr checkable = downtime->GetCheckable();

	if (!checkable->IsPaused())
		OnNotificationsRequested(checkable, NotificationDowntimeStart, checkable->GetLastCheckResult(), downtime->GetAuthor(), downtime->GetComment(), nullptr);
}

void Checkable::NotifyDowntimeEnd(const Downtime::Ptr& downtime)
{
	/* don't send notifications for flexible downtimes which never triggered */
	if (!downtime->GetFixed() && !downtime->IsTriggered())
		return;

	Checkable::Ptr checkable = downtime->GetCheckable();

	if (!checkable->IsPaused())
		OnNotificationsRequested(checkable, NotificationDowntimeEnd, checkable->GetLastCheckResult(), downtime->GetAuthor(), downtime->GetComment(), nullptr);
}

void Checkable::ValidateCheckInterval(const Lazy<double>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Checkable>::ValidateCheckInterval(lvalue, utils);

	if (lvalue() <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "check_interval" }, "Interval must be greater than 0."));
}

void Checkable::ValidateRetryInterval(const Lazy<double>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Checkable>::ValidateRetryInterval(lvalue, utils);

	if (lvalue() <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "retry_interval" }, "Interval must be greater than 0."));
}

void Checkable::ValidateMaxCheckAttempts(const Lazy<int>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Checkable>::ValidateMaxCheckAttempts(lvalue, utils);

	if (lvalue() <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "max_check_attempts" }, "Value must be greater than 0."));
}

void Checkable::CleanDeadlinedExecutions(const Timer * const&)
{
	double now = Utility::GetTime();
	Dictionary::Ptr executions;
	Dictionary::Ptr execution;

	for (auto& host : ConfigType::GetObjectsByType<Host>()) {
		executions = host->GetExecutions();
		if (executions) {
			for (const String& key : executions->GetKeys()) {
				execution = executions->Get(key);
				if (execution->Contains("deadline") && now > execution->Get("deadline")) {
					executions->Remove(key);
				}
			}
		}
	}

	for (auto& service : ConfigType::GetObjectsByType<Service>()) {
		executions = service->GetExecutions();
		if (executions) {
			for (const String& key : executions->GetKeys()) {
				execution = executions->Get(key);
				if (execution->Contains("deadline") && now > execution->Get("deadline")) {
					executions->Remove(key);
				}
			}
		}
	}
}
