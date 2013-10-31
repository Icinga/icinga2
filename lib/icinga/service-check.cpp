/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/utility.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

boost::signals2::signal<void (const Service::Ptr&, const Dictionary::Ptr&, const String&)> Service::OnNewCheckResult;
boost::signals2::signal<void (const Service::Ptr&, const Dictionary::Ptr&, StateType, const String&)> Service::OnStateChange;
boost::signals2::signal<void (const Service::Ptr&, NotificationType, const Dictionary::Ptr&, const String&, const String&)> Service::OnNotificationsRequested;
boost::signals2::signal<void (const Service::Ptr&, double, const String&)> Service::OnNextCheckChanged;
boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> Service::OnForceNextCheckChanged;
boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> Service::OnForceNextNotificationChanged;
boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> Service::OnEnableActiveChecksChanged;
boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> Service::OnEnablePassiveChecksChanged;
boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> Service::OnEnableNotificationsChanged;
boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> Service::OnEnableFlappingChanged;
boost::signals2::signal<void (const Service::Ptr&, FlappingState)> Service::OnFlappingChanged;

CheckCommand::Ptr Service::GetCheckCommand(void) const
{
	return CheckCommand::GetByName(GetCheckCommandRaw());
}

TimePeriod::Ptr Service::GetCheckPeriod(void) const
{
	return TimePeriod::GetByName(GetCheckPeriodRaw());
}

double Service::GetCheckInterval(void) const
{
	if (!GetOverrideCheckInterval().IsEmpty())
		return GetOverrideCheckInterval();
	else
		return GetCheckIntervalRaw();
}

void Service::SetCheckInterval(double interval)
{
	SetOverrideCheckInterval(interval);
}

double Service::GetRetryInterval(void) const
{
	if (!GetOverrideRetryInterval().IsEmpty())
		return GetOverrideRetryInterval();
	else
		return GetRetryIntervalRaw();
}

void Service::SetRetryInterval(double interval)
{
	SetOverrideRetryInterval(interval);
}

void Service::SetSchedulingOffset(long offset)
{
	m_SchedulingOffset = offset;
}

long Service::GetSchedulingOffset(void)
{
	return m_SchedulingOffset;
}

void Service::SetNextCheck(double nextCheck, const String& authority)
{
	SetNextCheckRaw(nextCheck);

	Utility::QueueAsyncCallback(boost::bind(boost::ref(Service::OnNextCheckChanged), GetSelf(), nextCheck, authority));
}

double Service::GetNextCheck(void)
{
	return GetNextCheckRaw();
}

void Service::UpdateNextCheck(void)
{
	ObjectLock olock(this);

	double interval;

	if (GetStateType() == StateTypeSoft)
		interval = GetRetryInterval();
	else
		interval = GetCheckInterval();

	double now = Utility::GetTime();
	double adj = 0;

	if (interval > 1)
		adj = fmod(now * 100 + GetSchedulingOffset(), interval * 100) / 100.0;

	SetNextCheck(now - adj + interval);
}

bool Service::HasBeenChecked(void) const
{
	return GetLastCheckResult() != NULL;
}

double Service::GetLastCheck(void) const
{
	Dictionary::Ptr cr = GetLastCheckResult();
	double schedule_end = -1;

	if (cr)
		schedule_end = cr->Get("schedule_end");

	return schedule_end;
}

bool Service::GetEnableActiveChecks(void) const
{
	if (!GetOverrideEnableActiveChecks().IsEmpty())
		return GetOverrideEnableActiveChecks();
	else
		return GetEnableActiveChecksRaw();
}

void Service::SetEnableActiveChecks(bool enabled, const String& authority)
{
	SetOverrideEnableActiveChecks(enabled);

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnEnableActiveChecksChanged), GetSelf(), enabled, authority));
}

bool Service::GetEnablePassiveChecks(void) const
{
	if (!GetOverrideEnablePassiveChecks().IsEmpty())
		return GetOverrideEnablePassiveChecks();
	else
		return GetEnablePassiveChecksRaw();
}

void Service::SetEnablePassiveChecks(bool enabled, const String& authority)
{
	SetOverrideEnablePassiveChecks(enabled);

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnEnablePassiveChecksChanged), GetSelf(), enabled, authority));
}

bool Service::GetForceNextCheck(void) const
{
	return GetForceNextCheckRaw();
}

void Service::SetForceNextCheck(bool forced, const String& authority)
{
	SetForceNextCheckRaw(forced);

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnForceNextCheckChanged), GetSelf(), forced, authority));
}

void Service::ProcessCheckResult(const Dictionary::Ptr& cr, const String& authority)
{
	double now = Utility::GetTime();

	if (!cr->Contains("schedule_start"))
		cr->Set("schedule_start", now);

	if (!cr->Contains("schedule_end"))
		cr->Set("schedule_end", now);

	if (!cr->Contains("execution_start"))
		cr->Set("execution_start", now);

	if (!cr->Contains("execution_end"))
		cr->Set("execution_end", now);

	String check_source = cr->Get("check_source");

	if (check_source.IsEmpty())
		cr->Set("check_source", authority);

	bool reachable = IsReachable();

	Host::Ptr host = GetHost();
	bool host_reachable = true;

	if (host)
		host_reachable = host->IsReachable();

	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	Dictionary::Ptr old_cr = GetLastCheckResult();
	ServiceState old_state = GetState();
	StateType old_stateType = GetStateType();
	long old_attempt = GetCheckAttempt();
	bool recovery;

	if (old_cr && cr->Get("execution_start") < old_cr->Get("execution_start"))
		return;

	/* The ExecuteCheck function already sets the old state, but we need to do it again
	 * in case this was a passive check result. */
	SetLastState(old_state);
	SetLastStateType(old_stateType);
	SetLastReachable(reachable);

	long attempt;

	if (cr->Get("state") == StateOK) {
		if (old_state == StateOK && old_stateType == StateTypeSoft)
			SetStateType(StateTypeHard); // SOFT OK -> HARD OK

		attempt = 1;
		recovery = true;
		ResetNotificationNumbers();
		SetLastStateOK(Utility::GetTime());
	} else {
		if (old_attempt >= GetMaxCheckAttempts()) {
			SetStateType(StateTypeHard);
			attempt = 1;
		} else if (GetStateType() == StateTypeSoft || GetState() == StateOK) {
			SetStateType(StateTypeSoft);
			attempt = old_attempt + 1;
		} else {
			attempt = old_attempt;
		}

		recovery = false;

		if (cr->Get("state") == StateWarning)
			SetLastStateWarning(Utility::GetTime());
		if (cr->Get("state") == StateCritical)
			SetLastStateCritical(Utility::GetTime());
		if (cr->Get("state") == StateUnknown)
			SetLastStateUnknown(Utility::GetTime());
	}

	if (!reachable)
		SetLastStateUnreachable(Utility::GetTime());

	SetCheckAttempt(attempt);

	int state = cr->Get("state");
	SetState(static_cast<ServiceState>(state));

	bool call_eventhandler = false;
	bool stateChange = (old_state != GetState());
	if (stateChange) {
		SetLastStateChange(now);

		/* remove acknowledgements */
		if (GetAcknowledgement() == AcknowledgementNormal ||
		    (GetAcknowledgement() == AcknowledgementSticky && GetStateType() == StateTypeHard && GetState() == StateOK)) {
			ClearAcknowledgement();
		}

		/* reschedule service dependencies */
		BOOST_FOREACH(const Service::Ptr& parent, GetParentServices()) {
			ObjectLock olock(parent);
			parent->SetNextCheck(Utility::GetTime());
		}

		/* reschedule host dependencies */
		BOOST_FOREACH(const Host::Ptr& parent, GetParentHosts()) {
			Service::Ptr service = parent->GetCheckService();

			if (service && service->GetName() != GetName()) {
				ObjectLock olock(service);
				service->SetNextCheck(Utility::GetTime());
			}
		}

		call_eventhandler = true;
	}

	bool remove_acknowledgement_comments = false;

	if (GetAcknowledgement() == AcknowledgementNone)
		remove_acknowledgement_comments = true;

	bool hardChange = (GetStateType() == StateTypeHard && old_stateType == StateTypeSoft);

	if (old_state != GetState() && old_stateType == StateTypeHard && GetStateType() == StateTypeHard)
		hardChange = true;

	if (GetVolatile())
		hardChange = true;

	if (hardChange) {
		SetLastHardState(GetState());
		SetLastHardStateChange(now);
	}

	if (GetState() != StateOK)
		TriggerDowntimes();

	Service::UpdateStatistics(cr);

	bool in_downtime = IsInDowntime();
	bool send_notification = hardChange && reachable && !in_downtime && !IsAcknowledged();

	if (old_state == StateOK && old_stateType == StateTypeSoft)
		send_notification = false; /* Don't send notifications for SOFT-OK -> HARD-OK. */

	bool send_downtime_notification = (GetLastInDowntime() != in_downtime);
	SetLastInDowntime(in_downtime);

	olock.Unlock();

	if (remove_acknowledgement_comments)
		RemoveCommentsByType(CommentAcknowledgement);

	Dictionary::Ptr vars_after = boost::make_shared<Dictionary>();
	vars_after->Set("state", GetState());
	vars_after->Set("state_type", GetStateType());
	vars_after->Set("attempt", GetCheckAttempt());
	vars_after->Set("reachable", reachable);
	vars_after->Set("host_reachable", host_reachable);

	if (old_cr)
		cr->Set("vars_before", old_cr->Get("vars_after"));

	cr->Set("vars_after", vars_after);

	olock.Lock();
	SetLastCheckResult(cr);

	bool was_flapping, is_flapping;

	was_flapping = IsFlapping();
	if (GetStateType() == StateTypeHard)
		UpdateFlappingStatus(stateChange);
	is_flapping = IsFlapping();

	olock.Unlock();

//	Log(LogDebug, "icinga", "Flapping: Service " + GetName() +
//			" was: " + Convert::ToString(was_flapping) +
//			" is: " + Convert::ToString(is_flapping) +
//			" threshold: " + Convert::ToString(GetFlappingThreshold()) +
//			"% current: " +	Convert::ToString(GetFlappingCurrent()) + "%.");

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnNewCheckResult), GetSelf(), cr, authority));
	OnStateChanged(GetSelf());

	if (hardChange) {
		Utility::QueueAsyncCallback(boost::bind(boost::ref(OnStateChange), GetSelf(), cr, StateTypeHard, authority));
	}
	else if (stateChange) {
		Utility::QueueAsyncCallback(boost::bind(boost::ref(OnStateChange), GetSelf(), cr, StateTypeSoft, authority));
	}

	if (call_eventhandler)
		ExecuteEventHandler();

	if (send_downtime_notification)
		OnNotificationsRequested(GetSelf(), in_downtime ? NotificationDowntimeStart : NotificationDowntimeEnd, cr, "", "");

	if (!was_flapping && is_flapping) {
		OnNotificationsRequested(GetSelf(), NotificationFlappingStart, cr, "", "");

		Log(LogDebug, "icinga", "Flapping: Service " + GetName() + " started flapping (" + Convert::ToString(GetFlappingThreshold()) + "% < " + Convert::ToString(GetFlappingCurrent()) + "%).");
		OnFlappingChanged(GetSelf(), FlappingStarted);
	} else if (was_flapping && !is_flapping) {
		OnNotificationsRequested(GetSelf(), NotificationFlappingEnd, cr, "", "");

		Log(LogDebug, "icinga", "Flapping: Service " + GetName() + " stopped flapping (" + Convert::ToString(GetFlappingThreshold()) + "% >= " + Convert::ToString(GetFlappingCurrent()) + "%).");
		OnFlappingChanged(GetSelf(), FlappingStopped);
	} else if (send_notification)
		OnNotificationsRequested(GetSelf(), recovery ? NotificationRecovery : NotificationProblem, cr, "", "");
}

ServiceState Service::StateFromString(const String& state)
{
	if (state == "OK")
		return StateOK;
	else if (state == "WARNING")
		return StateWarning;
	else if (state == "CRITICAL")
		return StateCritical;
	else
		return StateUnknown;
}

String Service::StateToString(ServiceState state)
{
	switch (state) {
		case StateOK:
			return "OK";
		case StateWarning:
			return "WARNING";
		case StateCritical:
			return "CRITICAL";
		case StateUnknown:
		default:
			return "UNKNOWN";
	}
}

StateType Service::StateTypeFromString(const String& type)
{
	if (type == "SOFT")
		return StateTypeSoft;
	else
		return StateTypeHard;
}

String Service::StateTypeToString(StateType type)
{
	if (type == StateTypeSoft)
		return "SOFT";
	else
		return "HARD";
}

void Service::ExecuteCheck(void)
{
	ASSERT(!OwnsLock());

	bool reachable = IsReachable();

	{
		ObjectLock olock(this);

		/* don't run another check if there is one pending */
		if (m_CheckRunning)
			return;

		m_CheckRunning = true;

		SetLastState(GetState());
		SetLastStateType(GetLastStateType());
		SetLastReachable(reachable);
	}

	/* keep track of scheduling info in case the check type doesn't provide its own information */
	Dictionary::Ptr checkInfo = boost::make_shared<Dictionary>();
	checkInfo->Set("schedule_start", GetNextCheck());
	checkInfo->Set("execution_start", Utility::GetTime());

	Service::Ptr self = GetSelf();

	Dictionary::Ptr result;

	try {
		CheckCommand::Ptr command = GetCheckCommand();

		if (!command) {
			Log(LogDebug, "icinga", "No check_command found for service '" + GetName() + "'. Skipping execution.");
			return;
		}

		result = command->Execute(GetSelf());
	} catch (const std::exception& ex) {
		std::ostringstream msgbuf;
		msgbuf << "Exception occured during check for service '"
		       << GetName() << "': " << boost::diagnostic_information(ex);
		String message = msgbuf.str();

		Log(LogWarning, "icinga", message);

		result = boost::make_shared<Dictionary>();
		result->Set("state", StateUnknown);
		result->Set("output", message);
	}

	checkInfo->Set("execution_end", Utility::GetTime());
	checkInfo->Set("schedule_end", Utility::GetTime());

	if (result) {
		if (!result->Contains("schedule_start"))
			result->Set("schedule_start", checkInfo->Get("schedule_start"));

		if (!result->Contains("schedule_end"))
			result->Set("schedule_end", checkInfo->Get("schedule_end"));

		if (!result->Contains("execution_start"))
			result->Set("execution_start", checkInfo->Get("execution_start"));

		if (!result->Contains("execution_end"))
			result->Set("execution_end", checkInfo->Get("execution_end"));

		if (!result->Contains("macros"))
			result->Set("macros", checkInfo->Get("macros"));

		if (!result->Contains("active"))
			result->Set("active", 1);
	}

	if (result)
		ProcessCheckResult(result);

	/* figure out when the next check is for this service; the call to
	 * ProcessCheckResult() should've already done this but lets do it again
	 * just in case there was no check result. */
	UpdateNextCheck();

	{
		ObjectLock olock(this);
		m_CheckRunning = false;
	}
}

void Service::UpdateStatistics(const Dictionary::Ptr& cr)
{
	time_t ts;
	Value schedule_end = cr->Get("schedule_end");
	if (!schedule_end.IsEmpty())
		ts = static_cast<time_t>(schedule_end);
	else
		ts = static_cast<time_t>(Utility::GetTime());

	Value active = cr->Get("active");
	if (active.IsEmpty() || static_cast<long>(active))
		CIB::UpdateActiveChecksStatistics(ts, 1);
	else
		CIB::UpdatePassiveChecksStatistics(ts, 1);
}

double Service::CalculateExecutionTime(const Dictionary::Ptr& cr)
{
	double execution_start = 0, execution_end = 0;

	if (cr) {
		if (!cr->Contains("execution_start") || !cr->Contains("execution_end"))
			return 0;

		execution_start = cr->Get("execution_start");
		execution_end = cr->Get("execution_end");
	}

	return (execution_end - execution_start);
}

double Service::CalculateLatency(const Dictionary::Ptr& cr)
{
	double schedule_start = 0, schedule_end = 0;

	if (cr) {
		if (!cr->Contains("schedule_start") || !cr->Contains("schedule_end"))
			return 0;

		schedule_start = cr->Get("schedule_start");
		schedule_end = cr->Get("schedule_end");
	}

	double latency = (schedule_end - schedule_start) - CalculateExecutionTime(cr);

	if (latency < 0)
		latency = 0;

	return latency;
}
