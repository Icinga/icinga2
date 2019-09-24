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

boost::signals2::signal<void (const Checkable::Ptr&, const String&, const String&, AcknowledgementType, bool, bool, double, const MessageOrigin::Ptr&)> Checkable::OnAcknowledgementSet;
boost::signals2::signal<void (const Checkable::Ptr&, const MessageOrigin::Ptr&)> Checkable::OnAcknowledgementCleared;

static Timer::Ptr l_CheckablesFireSuppressedNotifications;

void Checkable::StaticInitialize()
{
	/* fixed downtime start */
	Downtime::OnDowntimeStarted.connect(std::bind(&Checkable::NotifyFixedDowntimeStart, _1));
	/* flexible downtime start */
	Downtime::OnDowntimeTriggered.connect(std::bind(&Checkable::NotifyFlexibleDowntimeStart, _1));
	/* fixed/flexible downtime end */
	Downtime::OnDowntimeRemoved.connect(std::bind(&Checkable::NotifyDowntimeEnd, _1));
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
			ClearAcknowledgement();
		}
	}

	return avalue;
}

bool Checkable::IsAcknowledged() const
{
	return const_cast<Checkable *>(this)->GetAcknowledgement() != AcknowledgementNone;
}

void Checkable::AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, bool notify, bool persistent, double expiry, const MessageOrigin::Ptr& origin)
{
	SetAcknowledgementRaw(type);
	SetAcknowledgementExpiry(expiry);

	if (notify && !IsPaused())
		OnNotificationsRequested(this, NotificationAcknowledgement, GetLastCheckResult(), author, comment, nullptr);

	Log(LogInformation, "Checkable")
		<< "Acknowledgement set for checkable '" << GetName() << "'.";

	OnAcknowledgementSet(this, author, comment, type, notify, persistent, expiry, origin);
}

void Checkable::ClearAcknowledgement(const MessageOrigin::Ptr& origin)
{
	SetAcknowledgementRaw(AcknowledgementNone);
	SetAcknowledgementExpiry(0);

	Log(LogInformation, "Checkable")
		<< "Acknowledgement cleared for checkable '" << GetName() << "'.";

	OnAcknowledgementCleared(this, origin);
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
	return !IsStateOK(GetStateRaw());
}

bool Checkable::GetHandled() const
{
	return GetProblem() && (IsInDowntime() || IsAcknowledged());
}

Timestamp Checkable::GetNextUpdate() const
{
	auto cr (GetLastCheckResult());

	if (cr) {
		return GetNextCheck()
			+ (GetProblem() && GetStateType() == StateTypeSoft ? GetRetryInterval() : GetCheckInterval())
			+ 2 * (cr->GetExecutionEnd() - cr->GetScheduleStart());
	} else {
		return GetNextCheck() + GetCheckInterval();
	}
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
