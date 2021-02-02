/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/checkable.hpp"
#include "icinga/service.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/cib.hpp"
#include "icinga/clusterevents.hpp"
#include "remote/messageorigin.hpp"
#include "remote/apilistener.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/context.hpp"

using namespace icinga;

boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, const MessageOrigin::Ptr&)> Checkable::OnNewCheckResult;
boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, StateType, const MessageOrigin::Ptr&)> Checkable::OnStateChange;
boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, std::set<Checkable::Ptr>, const MessageOrigin::Ptr&)> Checkable::OnReachabilityChanged;
boost::signals2::signal<void (const Checkable::Ptr&, NotificationType, const CheckResult::Ptr&, const String&, const String&, const MessageOrigin::Ptr&)> Checkable::OnNotificationsRequested;
boost::signals2::signal<void (const Checkable::Ptr&)> Checkable::OnNextCheckUpdated;

Atomic<uint_fast64_t> Checkable::CurrentConcurrentChecks (0);

std::mutex Checkable::m_StatsMutex;
int Checkable::m_PendingChecks = 0;
std::condition_variable Checkable::m_PendingChecksCV;

CheckCommand::Ptr Checkable::GetCheckCommand() const
{
	return dynamic_pointer_cast<CheckCommand>(NavigateCheckCommandRaw());
}

TimePeriod::Ptr Checkable::GetCheckPeriod() const
{
	return TimePeriod::GetByName(GetCheckPeriodRaw());
}

void Checkable::SetSchedulingOffset(long offset)
{
	m_SchedulingOffset = offset;
}

long Checkable::GetSchedulingOffset()
{
	return m_SchedulingOffset;
}

void Checkable::UpdateNextCheck(const MessageOrigin::Ptr& origin)
{
	double interval;

	if (GetStateType() == StateTypeSoft && GetLastCheckResult() != nullptr)
		interval = GetRetryInterval();
	else
		interval = GetCheckInterval();

	double now = Utility::GetTime();
	double adj = 0;

	if (interval > 1)
		adj = fmod(now * 100 + GetSchedulingOffset(), interval * 100) / 100.0;

	if (adj != 0.0)
		adj = std::min(0.5 + fmod(GetSchedulingOffset(), interval * 5) / 100.0, adj);

	double nextCheck = now - adj + interval;
	double lastCheck = GetLastCheck();

	Log(LogDebug, "Checkable")
		<< "Update checkable '" << GetName() << "' with check interval '" << GetCheckInterval()
		<< "' from last check time at " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", (lastCheck < 0 ? 0 : lastCheck))
		<< " (" << GetLastCheck() << ") to next check time at " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", nextCheck) << " (" << nextCheck << ").";

	SetNextCheck(nextCheck, false, origin);
}

bool Checkable::HasBeenChecked() const
{
	return GetLastCheckResult() != nullptr;
}

double Checkable::GetLastCheck() const
{
	CheckResult::Ptr cr = GetLastCheckResult();
	double schedule_end = -1;

	if (cr)
		schedule_end = cr->GetScheduleEnd();

	return schedule_end;
}

void Checkable::ProcessCheckResult(const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin)
{
	{
		ObjectLock olock(this);
		m_CheckRunning = false;
	}

	if (!cr)
		return;

	double now = Utility::GetTime();

	if (cr->GetScheduleStart() == 0)
		cr->SetScheduleStart(now);

	if (cr->GetScheduleEnd() == 0)
		cr->SetScheduleEnd(now);

	if (cr->GetExecutionStart() == 0)
		cr->SetExecutionStart(now);

	if (cr->GetExecutionEnd() == 0)
		cr->SetExecutionEnd(now);

	if (!origin || origin->IsLocal())
		cr->SetCheckSource(IcingaApplication::GetInstance()->GetNodeName());

	Endpoint::Ptr command_endpoint = GetCommandEndpoint();

	/* override check source if command_endpoint was defined */
	if (command_endpoint && !GetExtension("agent_check"))
		cr->SetCheckSource(command_endpoint->GetName());

	/* agent checks go through the api */
	if (command_endpoint && GetExtension("agent_check")) {
		ApiListener::Ptr listener = ApiListener::GetInstance();

		if (listener) {
			/* send message back to its origin */
			Dictionary::Ptr message = ClusterEvents::MakeCheckResultMessage(this, cr);
			listener->SyncSendMessage(command_endpoint, message);
		}

		return;

	}

	if (!IsActive())
		return;

	bool reachable = IsReachable();
	bool notification_reachable = IsReachable(DependencyNotification);

	ObjectLock olock(this);

	CheckResult::Ptr old_cr = GetLastCheckResult();
	ServiceState old_state = GetStateRaw();
	StateType old_stateType = GetStateType();
	long old_attempt = GetCheckAttempt();
	bool recovery = false;

	/* When we have an check result already (not after fresh start),
	 * prevent to accept old check results and allow overrides for
	 * CRs happened in the future.
	 */
	if (old_cr) {
		double currentCRTimestamp = old_cr->GetExecutionStart();
		double newCRTimestamp = cr->GetExecutionStart();

		/* Our current timestamp may be from the future (wrong server time adjusted again). Allow overrides here. */
		if (currentCRTimestamp > now) {
			/* our current CR is from the future, let the new CR override it. */
			Log(LogDebug, "Checkable")
				<< std::fixed << std::setprecision(6) << "Processing check result for checkable '" << GetName() << "' from "
				<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", newCRTimestamp) << " (" << newCRTimestamp
				<< "). Overriding since ours is from the future at "
				<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", currentCRTimestamp) << " (" << currentCRTimestamp << ").";
		} else {
			/* Current timestamp is from the past, but the new timestamp is even more in the past. Skip it. */
			if (newCRTimestamp < currentCRTimestamp) {
				Log(LogDebug, "Checkable")
					<< std::fixed << std::setprecision(6) << "Skipping check result for checkable '" << GetName() << "' from "
					<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", newCRTimestamp) << " (" << newCRTimestamp
					<< "). It is in the past compared to ours at "
					<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", currentCRTimestamp) << " (" << currentCRTimestamp << ").";
				return;
			}
		}
	}

	/* The ExecuteCheck function already sets the old state, but we need to do it again
	 * in case this was a passive check result. */
	SetLastStateRaw(old_state);
	SetLastStateType(old_stateType);
	SetLastReachable(reachable);

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(this);

	CheckableType checkableType = CheckableHost;
	if (service)
		checkableType = CheckableService;

	long attempt = 1;

	std::set<Checkable::Ptr> children = GetChildren();

	if (IsStateOK(cr->GetState())) {
		SetStateType(StateTypeHard); // NOT-OK -> HARD OK

		if (!IsStateOK(old_state))
			recovery = true;

		ResetNotificationNumbers();
		SaveLastState(ServiceOK, cr->GetExecutionEnd());

		/* update reachability for child objects in OK state */
		if (!children.empty())
			OnReachabilityChanged(this, cr, children, origin);
	} else {
		/* OK -> NOT-OK change, first SOFT state. Reset attempt counter. */
		if (IsStateOK(old_state)) {
			SetStateType(StateTypeSoft);
			attempt = 1;
		}

		/* SOFT state change, increase attempt counter. */
		if (old_stateType == StateTypeSoft && !IsStateOK(old_state)) {
			SetStateType(StateTypeSoft);
			attempt = old_attempt + 1;
		}

		/* HARD state change (e.g. previously 2/3 and this next attempt). Reset attempt counter. */
		if (attempt >= GetMaxCheckAttempts()) {
			SetStateType(StateTypeHard);
			attempt = 1;
		}

		if (!IsStateOK(cr->GetState())) {
			SaveLastState(cr->GetState(), cr->GetExecutionEnd());
		}

		/* update reachability for child objects in NOT-OK state */
		if (!children.empty())
			OnReachabilityChanged(this, cr, children, origin);
	}

	if (recovery) {
		for (auto& child : children) {
			if (child->GetProblem() && child->GetEnableActiveChecks()) {
				auto nextCheck (now + Utility::Random() % 60);

				ObjectLock oLock (child);

				if (nextCheck < child->GetNextCheck()) {
					child->SetNextCheck(nextCheck);
				}
			}
		}
	}

	if (!reachable)
		SetLastStateUnreachable(cr->GetExecutionEnd());

	SetCheckAttempt(attempt);

	ServiceState new_state = cr->GetState();
	SetStateRaw(new_state);

	bool stateChange;

	/* Exception on state change calculation for hosts. */
	if (checkableType == CheckableService)
		stateChange = (old_state != new_state);
	else
		stateChange = (Host::CalculateState(old_state) != Host::CalculateState(new_state));

	/* Store the current last state change for the next iteration. */
	SetPreviousStateChange(GetLastStateChange());

	if (stateChange) {
		SetLastStateChange(cr->GetExecutionEnd());

		/* remove acknowledgements */
		if (GetAcknowledgement() == AcknowledgementNormal ||
			(GetAcknowledgement() == AcknowledgementSticky && IsStateOK(new_state))) {
			ClearAcknowledgement("");
		}

		/* reschedule direct parents */
		for (const Checkable::Ptr& parent : GetParents()) {
			if (parent.get() == this)
				continue;

			if (!parent->GetEnableActiveChecks())
				continue;

			if (parent->GetNextCheck() >= now + parent->GetRetryInterval()) {
				ObjectLock olock(parent);
				parent->SetNextCheck(now);
			}
		}
	}

	bool remove_acknowledgement_comments = false;

	if (GetAcknowledgement() == AcknowledgementNone)
		remove_acknowledgement_comments = true;

	bool hardChange = (GetStateType() == StateTypeHard && old_stateType == StateTypeSoft);

	if (stateChange && old_stateType == StateTypeHard && GetStateType() == StateTypeHard)
		hardChange = true;

	bool is_volatile = GetVolatile();

	if (hardChange || is_volatile) {
		SetLastHardStateRaw(new_state);
		SetLastHardStateChange(cr->GetExecutionEnd());
		SetLastHardStatesRaw(GetLastHardStatesRaw() / 100u + new_state * 100u);
	}

	if (stateChange) {
		SetLastSoftStatesRaw(GetLastSoftStatesRaw() / 100u + new_state * 100u);
	}

	if (!IsStateOK(new_state))
		TriggerDowntimes();

	/* statistics for external tools */
	Checkable::UpdateStatistics(cr, checkableType);

	bool in_downtime = IsInDowntime();

	bool send_notification = false;
	bool suppress_notification = !notification_reachable || in_downtime || IsAcknowledged();

	/* Send notifications whether when a hard state change occurred. */
	if (hardChange && !(old_stateType == StateTypeSoft && IsStateOK(new_state)))
		send_notification = true;
	/* Or if the checkable is volatile and in a HARD state. */
	else if (is_volatile && GetStateType() == StateTypeHard)
		send_notification = true;

	if (IsStateOK(old_state) && old_stateType == StateTypeSoft)
		send_notification = false; /* Don't send notifications for SOFT-OK -> HARD-OK. */

	if (is_volatile && IsStateOK(old_state) && IsStateOK(new_state))
		send_notification = false; /* Don't send notifications for volatile OK -> OK changes. */

	olock.Unlock();

	if (remove_acknowledgement_comments)
		RemoveCommentsByType(CommentAcknowledgement);

	Dictionary::Ptr vars_after = new Dictionary({
		{ "state", new_state },
		{ "state_type", GetStateType() },
		{ "attempt", GetCheckAttempt() },
		{ "reachable", reachable }
	});

	if (old_cr)
		cr->SetVarsBefore(old_cr->GetVarsAfter());

	cr->SetVarsAfter(vars_after);

	olock.Lock();

	if (service) {
		SetLastCheckResult(cr);
	} else {
		bool wasProblem = GetProblem();

		SetLastCheckResult(cr);

		if (GetProblem() != wasProblem) {
			auto services = host->GetServices();
			olock.Unlock();
			for (auto& service : services) {
				Service::OnHostProblemChanged(service, cr, origin);
			}
			olock.Lock();
		}
	}

	bool was_flapping = IsFlapping();

	UpdateFlappingStatus(old_state != cr->GetState());

	bool is_flapping = IsFlapping();

	if (cr->GetActive()) {
		UpdateNextCheck(origin);
	} else {
		/* Reschedule the next check for external passive check results. The side effect of
		 * this is that for as long as we receive results for a service we
		 * won't execute any active checks. */
		double offset;
		double ttl = cr->GetTtl();

		if (ttl > 0)
			offset = ttl;
		else
			offset = GetCheckInterval();

		SetNextCheck(Utility::GetTime() + offset, false, origin);
	}

	olock.Unlock();

#ifdef I2_DEBUG /* I2_DEBUG */
	Log(LogDebug, "Checkable")
		<< "Flapping: Checkable " << GetName()
		<< " was: " << was_flapping
		<< " is: " << is_flapping
		<< " threshold low: " << GetFlappingThresholdLow()
		<< " threshold high: " << GetFlappingThresholdHigh()
		<< "% current: " << GetFlappingCurrent() << "%.";
#endif /* I2_DEBUG */

	OnNewCheckResult(this, cr, origin);

	/* signal status updates to for example db_ido */
	OnStateChanged(this);

	String old_state_str = (service ? Service::StateToString(old_state) : Host::StateToString(Host::CalculateState(old_state)));
	String new_state_str = (service ? Service::StateToString(new_state) : Host::StateToString(Host::CalculateState(new_state)));

	/* Whether a hard state change or a volatile state change except OK -> OK happened. */
	if (hardChange || (is_volatile && !(IsStateOK(old_state) && IsStateOK(new_state)))) {
		OnStateChange(this, cr, StateTypeHard, origin);
		Log(LogNotice, "Checkable")
			<< "State Change: Checkable '" << GetName() << "' hard state change from " << old_state_str << " to " << new_state_str << " detected." << (is_volatile ? " Checkable is volatile." : "");
	}
	/* Whether a state change happened or the state type is SOFT (must be logged too). */
	else if (stateChange || GetStateType() == StateTypeSoft) {
		OnStateChange(this, cr, StateTypeSoft, origin);
		Log(LogNotice, "Checkable")
			<< "State Change: Checkable '" << GetName() << "' soft state change from " << old_state_str << " to " << new_state_str << " detected.";
	}

	if (GetStateType() == StateTypeSoft || hardChange || recovery ||
		(is_volatile && !(IsStateOK(old_state) && IsStateOK(new_state))))
		ExecuteEventHandler();

	int suppressed_types = 0;

	/* Flapping start/end notifications */
	if (!was_flapping && is_flapping) {
		/* FlappingStart notifications happen on state changes, not in downtimes */
		if (!IsPaused()) {
			if (in_downtime) {
				suppressed_types |= NotificationFlappingStart;
			} else {
				OnNotificationsRequested(this, NotificationFlappingStart, cr, "", "", nullptr);
			}
		}

		Log(LogNotice, "Checkable")
			<< "Flapping Start: Checkable '" << GetName() << "' started flapping (Current flapping value "
			<< GetFlappingCurrent() << "% > high threshold " << GetFlappingThresholdHigh() << "%).";

		NotifyFlapping(origin);
	} else if (was_flapping && !is_flapping) {
		/* FlappingEnd notifications are independent from state changes, must not happen in downtine */
		if (!IsPaused()) {
			if (in_downtime) {
				suppressed_types |= NotificationFlappingEnd;
			} else {
				OnNotificationsRequested(this, NotificationFlappingEnd, cr, "", "", nullptr);
			}
		}

		Log(LogNotice, "Checkable")
			<< "Flapping Stop: Checkable '" << GetName() << "' stopped flapping (Current flapping value "
			<< GetFlappingCurrent() << "% < low threshold " << GetFlappingThresholdLow() << "%).";

		NotifyFlapping(origin);
	}

	if (send_notification && !is_flapping) {
		if (!IsPaused()) {
			if (suppress_notification) {
				suppressed_types |= (recovery ? NotificationRecovery : NotificationProblem);
			} else {
				OnNotificationsRequested(this, recovery ? NotificationRecovery : NotificationProblem, cr, "", "", nullptr);
			}
		}
	}

	if (suppressed_types) {
		/* If some notifications were suppressed, but just because of e.g. a downtime,
		 * stash them into a notification types bitmask for maybe re-sending later.
		 */

		ObjectLock olock (this);
		int suppressed_types_before (GetSuppressedNotifications());
		int suppressed_types_after (suppressed_types_before | suppressed_types);

		for (int conflict : {NotificationProblem | NotificationRecovery, NotificationFlappingStart | NotificationFlappingEnd}) {
			/* E.g. problem and recovery notifications neutralize each other. */

			if ((suppressed_types_after & conflict) == conflict) {
				suppressed_types_after &= ~conflict;
			}
		}

		if (suppressed_types_after != suppressed_types_before) {
			SetSuppressedNotifications(suppressed_types_after);
		}
	}
}

void Checkable::ExecuteRemoteCheck(const Dictionary::Ptr& resolvedMacros)
{
	CONTEXT("Executing remote check for object '" + GetName() + "'");

	double scheduled_start = GetNextCheck();
	double before_check = Utility::GetTime();

	CheckResult::Ptr cr = new CheckResult();
	cr->SetScheduleStart(scheduled_start);
	cr->SetExecutionStart(before_check);

	GetCheckCommand()->Execute(this, cr, resolvedMacros, true);
}

void Checkable::ExecuteCheck()
{
	CONTEXT("Executing check for object '" + GetName() + "'");

	/* keep track of scheduling info in case the check type doesn't provide its own information */
	double scheduled_start = GetNextCheck();
	double before_check = Utility::GetTime();

	SetLastCheckStarted(Utility::GetTime());

	/* This calls SetNextCheck() which updates the CheckerComponent's idle/pending
	 * queues and ensures that checks are not fired multiple times. ProcessCheckResult()
	 * is called too late. See #6421.
	 */
	UpdateNextCheck();

	bool reachable = IsReachable();

	{
		ObjectLock olock(this);

		/* don't run another check if there is one pending */
		if (m_CheckRunning)
			return;

		m_CheckRunning = true;

		SetLastStateRaw(GetStateRaw());
		SetLastStateType(GetLastStateType());
		SetLastReachable(reachable);
	}

	CheckResult::Ptr cr = new CheckResult();

	cr->SetScheduleStart(scheduled_start);
	cr->SetExecutionStart(before_check);

	Endpoint::Ptr endpoint = GetCommandEndpoint();
	bool local = !endpoint || endpoint == Endpoint::GetLocalEndpoint();

	if (local) {
		GetCheckCommand()->Execute(this, cr, nullptr, false);
	} else {
		Dictionary::Ptr macros = new Dictionary();
		GetCheckCommand()->Execute(this, cr, macros, false);

		if (endpoint->GetConnected()) {
			/* perform check on remote endpoint */
			Dictionary::Ptr message = new Dictionary();
			message->Set("jsonrpc", "2.0");
			message->Set("method", "event::ExecuteCommand");

			Host::Ptr host;
			Service::Ptr service;
			tie(host, service) = GetHostService(this);

			Dictionary::Ptr params = new Dictionary();
			message->Set("params", params);
			params->Set("command_type", "check_command");
			params->Set("command", GetCheckCommand()->GetName());
			params->Set("host", host->GetName());

			if (service)
				params->Set("service", service->GetShortName());

			/*
			 * If the host/service object specifies the 'check_timeout' attribute,
			 * forward this to the remote endpoint to limit the command execution time.
			 */
			if (!GetCheckTimeout().IsEmpty())
				params->Set("check_timeout", GetCheckTimeout());

			params->Set("macros", macros);

			ApiListener::Ptr listener = ApiListener::GetInstance();

			if (listener)
				listener->SyncSendMessage(endpoint, message);

			/* Re-schedule the check so we don't run it again until after we've received
			 * a check result from the remote instance. The check will be re-scheduled
			 * using the proper check interval once we've received a check result.
			 */
			SetNextCheck(Utility::GetTime() + GetCheckCommand()->GetTimeout() + 30);

		/*
		 * Let the user know that there was a problem with the check if
		 * 1) The endpoint is not syncing (replay log, etc.)
		 * 2) Outside of the cold startup window (5min)
		 */
		} else if (!endpoint->GetSyncing() && Application::GetInstance()->GetStartTime() < Utility::GetTime() - 300) {
			/* fail to perform check on unconnected endpoint */
			cr->SetState(ServiceUnknown);

			String output = "Remote Icinga instance '" + endpoint->GetName() + "' is not connected to ";

			Endpoint::Ptr localEndpoint = Endpoint::GetLocalEndpoint();

			if (localEndpoint)
				output += "'" + localEndpoint->GetName() + "'";
			else
				output += "this instance";

			cr->SetOutput(output);

			ProcessCheckResult(cr);
		}

		{
			ObjectLock olock(this);
			m_CheckRunning = false;
		}
	}
}

void Checkable::UpdateStatistics(const CheckResult::Ptr& cr, CheckableType type)
{
	time_t ts = cr->GetScheduleEnd();

	if (type == CheckableHost) {
		if (cr->GetActive())
			CIB::UpdateActiveHostChecksStatistics(ts, 1);
		else
			CIB::UpdatePassiveHostChecksStatistics(ts, 1);
	} else if (type == CheckableService) {
		if (cr->GetActive())
			CIB::UpdateActiveServiceChecksStatistics(ts, 1);
		else
			CIB::UpdatePassiveServiceChecksStatistics(ts, 1);
	} else {
		Log(LogWarning, "Checkable", "Unknown checkable type for statistic update.");
	}
}

void Checkable::IncreasePendingChecks()
{
	std::unique_lock<std::mutex> lock(m_StatsMutex);
	m_PendingChecks++;
}

void Checkable::DecreasePendingChecks()
{
	std::unique_lock<std::mutex> lock(m_StatsMutex);
	m_PendingChecks--;
	m_PendingChecksCV.notify_one();
}

int Checkable::GetPendingChecks()
{
	std::unique_lock<std::mutex> lock(m_StatsMutex);
	return m_PendingChecks;
}

void Checkable::AquirePendingCheckSlot(int maxPendingChecks)
{
	std::unique_lock<std::mutex> lock(m_StatsMutex);
	while (m_PendingChecks >= maxPendingChecks)
		m_PendingChecksCV.wait(lock);

	m_PendingChecks++;
}
