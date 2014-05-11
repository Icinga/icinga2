/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/checkable.h"
#include "icinga/checkcommand.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include "remote/messageorigin.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/context.h"
#include <boost/foreach.hpp>

using namespace icinga;

boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, const MessageOrigin&)> Checkable::OnNewCheckResult;
boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, StateType, const MessageOrigin&)> Checkable::OnStateChange;
boost::signals2::signal<void (const Checkable::Ptr&, NotificationType, const CheckResult::Ptr&, const String&, const String&)> Checkable::OnNotificationsRequested;
boost::signals2::signal<void (const Checkable::Ptr&, double, const MessageOrigin&)> Checkable::OnNextCheckChanged;
boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> Checkable::OnForceNextCheckChanged;
boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> Checkable::OnForceNextNotificationChanged;
boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> Checkable::OnEnableActiveChecksChanged;
boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> Checkable::OnEnablePassiveChecksChanged;
boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> Checkable::OnEnableNotificationsChanged;
boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> Checkable::OnEnableFlappingChanged;
boost::signals2::signal<void (const Checkable::Ptr&, FlappingState)> Checkable::OnFlappingChanged;

CheckCommand::Ptr Checkable::GetCheckCommand(void) const
{
	String command;

	if (!GetOverrideCheckCommand().IsEmpty())
		command = GetOverrideCheckCommand();
	else
		command = GetCheckCommandRaw();

	return CheckCommand::GetByName(command);
}

void Checkable::SetCheckCommand(const CheckCommand::Ptr& command)
{
	SetOverrideCheckCommand(command->GetName());
}

TimePeriod::Ptr Checkable::GetCheckPeriod(void) const
{
	String tp;

	if (!GetOverrideCheckPeriod().IsEmpty())
		tp = GetOverrideCheckPeriod();
	else
		tp = GetCheckPeriodRaw();

	return TimePeriod::GetByName(tp);
}

void Checkable::SetCheckPeriod(const TimePeriod::Ptr& tp)
{
	SetOverrideCheckPeriod(tp->GetName());
}

double Checkable::GetCheckInterval(void) const
{
	if (!GetOverrideCheckInterval().IsEmpty())
		return GetOverrideCheckInterval();
	else
		return GetCheckIntervalRaw();
}

void Checkable::SetCheckInterval(double interval)
{
	SetOverrideCheckInterval(interval);
}

double Checkable::GetRetryInterval(void) const
{
	if (!GetOverrideRetryInterval().IsEmpty())
		return GetOverrideRetryInterval();
	else
		return GetRetryIntervalRaw();
}

void Checkable::SetRetryInterval(double interval)
{
	SetOverrideRetryInterval(interval);
}

void Checkable::SetSchedulingOffset(long offset)
{
	m_SchedulingOffset = offset;
}

long Checkable::GetSchedulingOffset(void)
{
	return m_SchedulingOffset;
}

void Checkable::SetNextCheck(double nextCheck, const MessageOrigin& origin)
{
	SetNextCheckRaw(nextCheck);

	OnNextCheckChanged(GetSelf(), nextCheck, origin);
}

double Checkable::GetNextCheck(void)
{
	return GetNextCheckRaw();
}

void Checkable::UpdateNextCheck(void)
{
	ObjectLock olock(this);

	double interval;

	if (GetStateType() == StateTypeSoft && GetLastCheckResult() != NULL)
		interval = GetRetryInterval();
	else
		interval = GetCheckInterval();

	double now = Utility::GetTime();
	double adj = 0;

	if (interval > 1)
		adj = fmod(now * 100 + GetSchedulingOffset(), interval * 100) / 100.0;

	SetNextCheck(now - adj + interval);
}

bool Checkable::HasBeenChecked(void) const
{
	return GetLastCheckResult() != NULL;
}

double Checkable::GetLastCheck(void) const
{
	CheckResult::Ptr cr = GetLastCheckResult();
	double schedule_end = -1;

	if (cr)
		schedule_end = cr->GetScheduleEnd();

	return schedule_end;
}

bool Checkable::GetEnableActiveChecks(void) const
{
	if (!GetOverrideEnableActiveChecks().IsEmpty())
		return GetOverrideEnableActiveChecks();
	else
		return GetEnableActiveChecksRaw();
}

void Checkable::SetEnableActiveChecks(bool enabled, const MessageOrigin& origin)
{
	SetOverrideEnableActiveChecks(enabled);

	OnEnableActiveChecksChanged(GetSelf(), enabled, origin);
}

bool Checkable::GetEnablePassiveChecks(void) const
{
	if (!GetOverrideEnablePassiveChecks().IsEmpty())
		return GetOverrideEnablePassiveChecks();
	else
		return GetEnablePassiveChecksRaw();
}

void Checkable::SetEnablePassiveChecks(bool enabled, const MessageOrigin& origin)
{
	SetOverrideEnablePassiveChecks(enabled);

	OnEnablePassiveChecksChanged(GetSelf(), enabled, origin);
}

bool Checkable::GetForceNextCheck(void) const
{
	return GetForceNextCheckRaw();
}

void Checkable::SetForceNextCheck(bool forced, const MessageOrigin& origin)
{
	SetForceNextCheckRaw(forced);

	OnForceNextCheckChanged(GetSelf(), forced, origin);
}

int Checkable::GetMaxCheckAttempts(void) const
{
	if (!GetOverrideMaxCheckAttempts().IsEmpty())
		return GetOverrideMaxCheckAttempts();
	else
		return GetMaxCheckAttemptsRaw();
}

void Checkable::SetMaxCheckAttempts(int attempts)
{
	SetOverrideMaxCheckAttempts(attempts);
}

void Checkable::ProcessCheckResult(const CheckResult::Ptr& cr, const MessageOrigin& origin)
{
	{
		ObjectLock olock(this);
		m_CheckRunning = false;
	}

	double now = Utility::GetTime();

	if (cr->GetScheduleStart() == 0)
		cr->SetScheduleStart(now);

	if (cr->GetScheduleEnd() == 0)
		cr->SetScheduleEnd(now);

	if (cr->GetExecutionStart() == 0)
		cr->SetExecutionStart(now);

	if (cr->GetExecutionEnd() == 0)
		cr->SetExecutionEnd(now);

	if (origin.IsLocal())
		cr->SetCheckSource(IcingaApplication::GetInstance()->GetNodeName());

	bool reachable = IsReachable();
	bool notification_reachable = IsReachable(DependencyNotification);

	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	CheckResult::Ptr old_cr = GetLastCheckResult();
	ServiceState old_state = GetStateRaw();
	StateType old_stateType = GetStateType();
	long old_attempt = GetCheckAttempt();
	bool recovery;

	if (old_cr && cr->GetExecutionStart() < old_cr->GetExecutionStart())
		return;

	/* The ExecuteCheck function already sets the old state, but we need to do it again
	 * in case this was a passive check result. */
	SetLastStateRaw(old_state);
	SetLastStateType(old_stateType);
	SetLastReachable(reachable);

	long attempt = 1;

	if (!old_cr) {
		SetStateType(StateTypeHard);
	} else if (cr->GetState() == ServiceOK) {
		if (old_state == ServiceOK && old_stateType == StateTypeSoft)
			SetStateType(StateTypeHard); // SOFT OK -> HARD OK

		recovery = true;
		ResetNotificationNumbers();
		SetLastStateOK(Utility::GetTime());
	} else {
		if (old_attempt >= GetMaxCheckAttempts()) {
			SetStateType(StateTypeHard);
		} else if (old_stateType == StateTypeSoft || old_state == ServiceOK) {
			SetStateType(StateTypeSoft);
			attempt = old_attempt + 1;
		} else {
			attempt = old_attempt;
		}

		recovery = false;

		switch (cr->GetState()) {
			case ServiceOK:
				/* Nothing to do here. */
				break;
			case ServiceWarning:
				SetLastStateWarning(Utility::GetTime());
				break;
			case ServiceCritical:
				SetLastStateCritical(Utility::GetTime());
				break;
			case ServiceUnknown:
				SetLastStateUnknown(Utility::GetTime());
				break;
		}
	}

	if (!reachable)
		SetLastStateUnreachable(Utility::GetTime());

	SetCheckAttempt(attempt);

	ServiceState new_state = cr->GetState();
	SetStateRaw(new_state);

	bool stateChange = (old_state != new_state);
	if (stateChange) {
		SetLastStateChange(now);

		/* remove acknowledgements */
		if (GetAcknowledgement() == AcknowledgementNormal ||
		    (GetAcknowledgement() == AcknowledgementSticky && new_state == ServiceOK)) {
			ClearAcknowledgement();
		}

		/* reschedule direct parents */
		BOOST_FOREACH(const Checkable::Ptr& parent, GetParents()) {
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

	if (GetVolatile())
		hardChange = true;

	if (hardChange) {
		SetLastHardStateRaw(new_state);
		SetLastHardStateChange(now);
	}

	if (new_state != ServiceOK)
		TriggerDowntimes();

	Checkable::UpdateStatistics(cr);

	bool in_downtime = IsInDowntime();
	bool send_notification = hardChange && notification_reachable && !in_downtime && !IsAcknowledged();

	if (!old_cr)
		send_notification = false; /* Don't send notifications for the initial state change */

	if (old_state == ServiceOK && old_stateType == StateTypeSoft)
		send_notification = false; /* Don't send notifications for SOFT-OK -> HARD-OK. */

	bool send_downtime_notification = (GetLastInDowntime() != in_downtime);
	SetLastInDowntime(in_downtime);

	olock.Unlock();

	if (remove_acknowledgement_comments)
		RemoveCommentsByType(CommentAcknowledgement);

	Dictionary::Ptr vars_after = make_shared<Dictionary>();
	vars_after->Set("state", new_state);
	vars_after->Set("state_type", GetStateType());
	vars_after->Set("attempt", GetCheckAttempt());
	vars_after->Set("reachable", reachable);

	if (old_cr)
		cr->SetVarsBefore(old_cr->GetVarsAfter());

	cr->SetVarsAfter(vars_after);

	olock.Lock();
	SetLastCheckResult(cr);

	bool was_flapping, is_flapping;

	was_flapping = IsFlapping();
	if (GetStateType() == StateTypeHard)
		UpdateFlappingStatus(stateChange);
	is_flapping = IsFlapping();

	olock.Unlock();

//	Log(LogDebug, "icinga", "Flapping: Checkable " + GetName() +
//			" was: " + Convert::ToString(was_flapping) +
//			" is: " + Convert::ToString(is_flapping) +
//			" threshold: " + Convert::ToString(GetFlappingThreshold()) +
//			"% current: " +	Convert::ToString(GetFlappingCurrent()) + "%.");

	OnNewCheckResult(GetSelf(), cr, origin);

	/* signal status updates to for example db_ido */
	OnStateChanged(GetSelf());

	if (hardChange)
		OnStateChange(GetSelf(), cr, StateTypeHard, origin);
	else if (stateChange)
		OnStateChange(GetSelf(), cr, StateTypeSoft, origin);

	if (GetStateType() == StateTypeSoft || hardChange || recovery)
		ExecuteEventHandler();

	if (send_downtime_notification)
		OnNotificationsRequested(GetSelf(), in_downtime ? NotificationDowntimeStart : NotificationDowntimeEnd, cr, "", "");

	if (!was_flapping && is_flapping) {
		OnNotificationsRequested(GetSelf(), NotificationFlappingStart, cr, "", "");

		Log(LogDebug, "icinga", "Flapping: Checkable " + GetName() + " started flapping (" + Convert::ToString(GetFlappingThreshold()) + "% < " + Convert::ToString(GetFlappingCurrent()) + "%).");
		OnFlappingChanged(GetSelf(), FlappingStarted);
	} else if (was_flapping && !is_flapping) {
		OnNotificationsRequested(GetSelf(), NotificationFlappingEnd, cr, "", "");

		Log(LogDebug, "icinga", "Flapping: Checkable " + GetName() + " stopped flapping (" + Convert::ToString(GetFlappingThreshold()) + "% >= " + Convert::ToString(GetFlappingCurrent()) + "%).");
		OnFlappingChanged(GetSelf(), FlappingStopped);
	} else if (send_notification)
		OnNotificationsRequested(GetSelf(), recovery ? NotificationRecovery : NotificationProblem, cr, "", "");
}

bool Checkable::IsCheckPending(void) const
{
	ObjectLock olock(this);
	return m_CheckRunning;
}

void Checkable::ExecuteCheck(void)
{
	CONTEXT("Executing check for object '" + GetName() + "'");

	ASSERT(!OwnsLock());

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

	/* keep track of scheduling info in case the check type doesn't provide its own information */
	double scheduled_start = GetNextCheck();
	double before_check = Utility::GetTime();

	Checkable::Ptr self = GetSelf();

	CheckResult::Ptr result = make_shared<CheckResult>();

	result->SetScheduleStart(scheduled_start);
	result->SetExecutionStart(before_check);

	GetCheckCommand()->Execute(GetSelf(), result);
}

void Checkable::UpdateStatistics(const CheckResult::Ptr& cr)
{
	time_t ts = cr->GetScheduleEnd();

	if (cr->GetActive())
		CIB::UpdateActiveChecksStatistics(ts, 1);
	else
		CIB::UpdatePassiveChecksStatistics(ts, 1);
}

double Checkable::CalculateExecutionTime(const CheckResult::Ptr& cr)
{
	if (!cr)
		return 0;

	return cr->GetExecutionEnd() - cr->GetExecutionStart();
}

double Checkable::CalculateLatency(const CheckResult::Ptr& cr)
{
	if (!cr)
		return 0;

	double latency = (cr->GetScheduleEnd() - cr->GetScheduleStart()) - CalculateExecutionTime(cr);

	if (latency < 0)
		latency = 0;

	return latency;
}
