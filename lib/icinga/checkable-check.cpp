/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

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

boost::mutex Checkable::m_StatsMutex;
int Checkable::m_PendingChecks = 0;

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

	adj = std::min(0.5 + fmod(GetSchedulingOffset(), interval * 5) / 100.0, adj);

	SetNextCheck(now - adj + interval, false, origin);
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

	bool reachable = IsReachable();
	bool notification_reachable = IsReachable(DependencyNotification);

	ObjectLock olock(this);

	CheckResult::Ptr old_cr = GetLastCheckResult();
	ServiceState old_state = GetStateRaw();
	StateType old_stateType = GetStateType();
	long old_attempt = GetCheckAttempt();
	bool recovery = false;

	/* Ignore check results older than the current one. */
	if (old_cr && cr->GetExecutionStart() < old_cr->GetExecutionStart())
		return;

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
		SaveLastState(ServiceOK, Utility::GetTime());

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
			SaveLastState(cr->GetState(), Utility::GetTime());
		}

		/* update reachability for child objects in NOT-OK state */
		if (!children.empty())
			OnReachabilityChanged(this, cr, children, origin);
	}

	if (!reachable)
		SetLastStateUnreachable(Utility::GetTime());

	SetCheckAttempt(attempt);

	ServiceState new_state = cr->GetState();
	SetStateRaw(new_state);

	bool stateChange;

	/* Exception on state change calculation for hosts. */
	if (checkableType == CheckableService)
		stateChange = (old_state != new_state);
	else
		stateChange = (Host::CalculateState(old_state) != Host::CalculateState(new_state));

	if (stateChange) {
		SetLastStateChange(now);

		/* remove acknowledgements */
		if (GetAcknowledgement() == AcknowledgementNormal ||
			(GetAcknowledgement() == AcknowledgementSticky && IsStateOK(new_state))) {
			ClearAcknowledgement();
		}

		/* reschedule direct parents */
		for (const Checkable::Ptr& parent : GetParents()) {
			if (parent.get() == this)
				continue;

			ObjectLock olock(parent);
			parent->SetNextCheck(Utility::GetTime());
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
		SetLastHardStateChange(now);
	}

	if (!IsStateOK(new_state))
		TriggerDowntimes();

	/* statistics for external tools */
	Checkable::UpdateStatistics(cr, checkableType);

	bool in_downtime = IsInDowntime();

	bool send_notification = false;

	if (notification_reachable && !in_downtime && !IsAcknowledged()) {
		/* Send notifications whether when a hard state change occured. */
		if (hardChange && !(old_stateType == StateTypeSoft && IsStateOK(new_state)))
			send_notification = true;
		/* Or if the checkable is volatile and in a HARD state. */
		else if (is_volatile && GetStateType() == StateTypeHard)
			send_notification = true;
	}

	if (IsStateOK(old_state) && old_stateType == StateTypeSoft)
		send_notification = false; /* Don't send notifications for SOFT-OK -> HARD-OK. */

	if (is_volatile && IsStateOK(old_state) && IsStateOK(new_state))
		send_notification = false; /* Don't send notifications for volatile OK -> OK changes. */

	olock.Unlock();

	if (remove_acknowledgement_comments)
		RemoveCommentsByType(CommentAcknowledgement);

	Dictionary::Ptr vars_after = new Dictionary();
	vars_after->Set("state", new_state);
	vars_after->Set("state_type", GetStateType());
	vars_after->Set("attempt", GetCheckAttempt());
	vars_after->Set("reachable", reachable);

	if (old_cr)
		cr->SetVarsBefore(old_cr->GetVarsAfter());

	cr->SetVarsAfter(vars_after);

	olock.Lock();
	SetLastCheckResult(cr);

	bool was_flapping = IsFlapping();

	UpdateFlappingStatus(old_state != cr->GetState());

	bool is_flapping = IsFlapping();

	if (cr->GetActive()) {
		UpdateNextCheck(origin);
	} else {
		/* Reschedule the next check for passive check results. The side effect of
		 * this is that for as long as we receive passive results for a service we
		 * won't execute any active checks. */
		SetNextCheck(Utility::GetTime() + GetCheckInterval(), false, origin);
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

	/* Flapping start/end notifications */
	if (!in_downtime && !was_flapping && is_flapping) {
		/* FlappingStart notifications happen on state changes, not in downtimes */
		if (!IsPaused())
			OnNotificationsRequested(this, NotificationFlappingStart, cr, "", "", nullptr);

		Log(LogNotice, "Checkable")
			<< "Flapping Start: Checkable '" << GetName() << "' started flapping (Current flapping value "
			<< GetFlappingCurrent() << "% > high threshold " << GetFlappingThresholdHigh() << "%).";

		NotifyFlapping(origin);
	} else if (!in_downtime && was_flapping && !is_flapping) {
		/* FlappingEnd notifications are independent from state changes, must not happen in downtine */
		if (!IsPaused())
			OnNotificationsRequested(this, NotificationFlappingEnd, cr, "", "", nullptr);

		Log(LogNotice, "Checkable")
			<< "Flapping Stop: Checkable '" << GetName() << "' stopped flapping (Current flapping value "
			<< GetFlappingCurrent() << "% < low threshold " << GetFlappingThresholdLow() << "%).";

		NotifyFlapping(origin);
	}

	if (send_notification && !is_flapping) {
		if (!IsPaused())
			OnNotificationsRequested(this, recovery ? NotificationRecovery : NotificationProblem, cr, "", "", nullptr);
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

			params->Set("macros", macros);

			ApiListener::Ptr listener = ApiListener::GetInstance();

			if (listener)
				listener->SyncSendMessage(endpoint, message);

			/* Re-schedule the check so we don't run it again until after we've received
			 * a check result from the remote instance. The check will be re-scheduled
			 * using the proper check interval once we've received a check result.
			 */
			SetNextCheck(Utility::GetTime() + GetCheckCommand()->GetTimeout() + 30);
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
	boost::mutex::scoped_lock lock(m_StatsMutex);
	m_PendingChecks++;
}

void Checkable::DecreasePendingChecks()
{
	boost::mutex::scoped_lock lock(m_StatsMutex);
	m_PendingChecks--;
}

int Checkable::GetPendingChecks()
{
	boost::mutex::scoped_lock lock(m_StatsMutex);
	return m_PendingChecks;
}
