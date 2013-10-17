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

const int Service::DefaultMaxCheckAttempts = 3;
const double Service::DefaultCheckInterval = 5 * 60;
const double Service::CheckIntervalDivisor = 5.0;

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
	return CheckCommand::GetByName(m_CheckCommand);
}

long Service::GetMaxCheckAttempts(void) const
{
	if (m_MaxCheckAttempts.IsEmpty())
		return DefaultMaxCheckAttempts;

	return m_MaxCheckAttempts;
}

TimePeriod::Ptr Service::GetCheckPeriod(void) const
{
	return TimePeriod::GetByName(m_CheckPeriod);
}

double Service::GetCheckInterval(void) const
{
	if (!m_OverrideCheckInterval.IsEmpty())
		return m_OverrideCheckInterval;
	else if (!m_CheckInterval.IsEmpty())
		return m_CheckInterval;
	else
		return DefaultCheckInterval;
}

void Service::SetCheckInterval(double interval)
{
	m_OverrideCheckInterval = interval;
}


double Service::GetRetryInterval(void) const
{
	if (!m_OverrideRetryInterval.IsEmpty())
		return m_OverrideRetryInterval;
	if (!m_RetryInterval.IsEmpty())
		return m_RetryInterval;
	else
		return GetCheckInterval() / CheckIntervalDivisor;
}

void Service::SetRetryInterval(double interval)
{
	m_OverrideRetryInterval = interval;
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
	m_NextCheck = nextCheck;

	Utility::QueueAsyncCallback(boost::bind(boost::ref(Service::OnNextCheckChanged), GetSelf(), nextCheck, authority));
}

double Service::GetNextCheck(void)
{
	return m_NextCheck;
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

void Service::SetCurrentChecker(const String& checker)
{
	m_CurrentChecker = checker;
}

String Service::GetCurrentChecker(void) const
{
	return m_CurrentChecker;
}

void Service::SetCurrentCheckAttempt(long attempt)
{
	m_CheckAttempt = attempt;
}

long Service::GetCurrentCheckAttempt(void) const
{
	if (m_CheckAttempt.IsEmpty())
		return 1;

	return m_CheckAttempt;
}

void Service::SetState(ServiceState state)
{
	m_State = static_cast<long>(state);
}

ServiceState Service::GetState(void) const
{
	if (m_State.IsEmpty())
		return StateUnknown;

	int ivalue = static_cast<int>(m_State);
	return static_cast<ServiceState>(ivalue);
}

void Service::SetLastState(ServiceState state)
{
	m_LastState = static_cast<long>(state);
}

ServiceState Service::GetLastState(void) const
{
	if (m_LastState.IsEmpty())
		return StateUnknown;

	int ivalue = static_cast<int>(m_LastState);
	return static_cast<ServiceState>(ivalue);
}

void Service::SetLastHardState(ServiceState state)
{
	m_LastHardState = static_cast<long>(state);
}

ServiceState Service::GetLastHardState(void) const
{
	if (m_LastHardState.IsEmpty())
		return StateUnknown;

	int ivalue = static_cast<int>(m_LastHardState);
	return static_cast<ServiceState>(ivalue);
}

void Service::SetStateType(StateType type)
{
	m_StateType = static_cast<long>(type);
}

StateType Service::GetStateType(void) const
{
	if (m_StateType.IsEmpty())
		return StateTypeSoft;

	int ivalue = static_cast<int>(m_StateType);
	return static_cast<StateType>(ivalue);
}

void Service::SetLastStateType(StateType type)
{
	m_LastStateType = static_cast<long>(type);
}

StateType Service::GetLastStateType(void) const
{
	if (m_LastStateType.IsEmpty())
		return StateTypeSoft;

	int ivalue = static_cast<int>(m_LastStateType);
	return static_cast<StateType>(ivalue);
}

void Service::SetLastStateOK(double ts)
{
	m_LastStateOK = ts;
}

double Service::GetLastStateOK(void) const
{
	if (m_LastStateOK.IsEmpty())
		return 0;

	return m_LastStateOK;
}

void Service::SetLastStateWarning(double ts)
{
	m_LastStateWarning = ts;
}

double Service::GetLastStateWarning(void) const
{
	if (m_LastStateWarning.IsEmpty())
		return 0;

	return m_LastStateWarning;
}

void Service::SetLastStateCritical(double ts)
{
	m_LastStateCritical = ts;
}

double Service::GetLastStateCritical(void) const
{
	if (m_LastStateCritical.IsEmpty())
		return 0;

	return m_LastStateCritical;
}

void Service::SetLastStateUnknown(double ts)
{
	m_LastStateUnknown = ts;
}

double Service::GetLastStateUnknown(void) const
{
	if (m_LastStateUnknown.IsEmpty())
		return 0;

	return m_LastStateUnknown;
}

void Service::SetLastStateUnreachable(double ts)
{
	m_LastStateUnreachable = ts;
}

double Service::GetLastStateUnreachable(void) const
{
	if (m_LastStateUnreachable.IsEmpty())
		return 0;

	return m_LastStateUnreachable;
}

void Service::SetLastReachable(bool reachable)
{
	m_LastReachable = reachable;
}

bool Service::GetLastReachable(void) const
{
	if (m_LastReachable.IsEmpty())
		return true;

	return m_LastReachable;
}

void Service::SetLastCheckResult(const Dictionary::Ptr& result)
{
	m_LastResult = result;
}

Dictionary::Ptr Service::GetLastCheckResult(void) const
{
	return m_LastResult;
}

bool Service::HasBeenChecked(void) const
{
	return GetLastCheckResult();
}

double Service::GetLastCheck(void) const
{
	Dictionary::Ptr cr = GetLastCheckResult();
	double schedule_end = -1;

	if (cr) {
		schedule_end = cr->Get("schedule_end");
	}

	return schedule_end;
}

String Service::GetLastCheckOutput(void) const
{
	Dictionary::Ptr cr = GetLastCheckResult();
	String output;

	if (cr) {
		String raw_output = cr->Get("output");
		size_t line_end = raw_output.Find("\n");
		output = raw_output.SubStr(0, line_end);
	}

	return output;
}

String Service::GetLastCheckLongOutput(void) const
{
	Dictionary::Ptr cr = GetLastCheckResult();
	String long_output;

	if (cr) {
		String raw_output = cr->Get("output");
		size_t line_end = raw_output.Find("\n");

		if (line_end > 0 && line_end != String::NPos) {
			long_output = raw_output.SubStr(line_end + 1, raw_output.GetLength());
			boost::algorithm::replace_all(long_output, "\n", "\\n");
		}
	}

	return long_output;
}

String Service::GetLastCheckPerfData(void) const
{
	Dictionary::Ptr cr = GetLastCheckResult();
	String perfdata;

	if (cr) {
		perfdata = cr->Get("performance_data_raw");

		boost::algorithm::replace_all(perfdata, "\n", "\\n");
	}

	return perfdata;
}

void Service::SetLastStateChange(double ts)
{
	m_LastStateChange = ts;
}

double Service::GetLastStateChange(void) const
{
	if (m_LastStateChange.IsEmpty())
		return IcingaApplication::GetInstance()->GetStartTime();

	return m_LastStateChange;
}

void Service::SetLastHardStateChange(double ts)
{
	m_LastHardStateChange = ts;
}

double Service::GetLastHardStateChange(void) const
{
	if (m_LastHardStateChange.IsEmpty())
		return IcingaApplication::GetInstance()->GetStartTime();

	return m_LastHardStateChange;
}

bool Service::GetEnableActiveChecks(void) const
{
	if (!m_OverrideEnableActiveChecks.IsEmpty())
		return m_OverrideEnableActiveChecks;
	else if (!m_EnableActiveChecks.IsEmpty())
		return m_EnableActiveChecks;
	else
		return true;
}

void Service::SetEnableActiveChecks(bool enabled, const String& authority)
{
	m_OverrideEnableActiveChecks = enabled ? 1 : 0;

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnEnableActiveChecksChanged), GetSelf(), enabled, authority));
}

bool Service::GetEnablePassiveChecks(void) const
{
	if (!m_OverrideEnablePassiveChecks.IsEmpty())
		return m_OverrideEnablePassiveChecks;
	if (!m_EnablePassiveChecks.IsEmpty())
		return m_EnablePassiveChecks;
	else
		return true;
}

void Service::SetEnablePassiveChecks(bool enabled, const String& authority)
{
	m_OverrideEnablePassiveChecks = enabled ? 1 : 0;

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnEnablePassiveChecksChanged), GetSelf(), enabled, authority));
}

bool Service::GetForceNextCheck(void) const
{
	if (m_ForceNextCheck.IsEmpty())
		return false;

	return static_cast<bool>(m_ForceNextCheck);
}

void Service::SetForceNextCheck(bool forced, const String& authority)
{
	m_ForceNextCheck = forced ? 1 : 0;

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
	long old_attempt = GetCurrentCheckAttempt();
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

	SetCurrentCheckAttempt(attempt);

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

	if (IsVolatile())
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

	bool send_downtime_notification = m_LastInDowntime != in_downtime;
	m_LastInDowntime = in_downtime;

	olock.Unlock();

	if (remove_acknowledgement_comments)
		RemoveCommentsByType(CommentAcknowledgement);

	Dictionary::Ptr vars_after = boost::make_shared<Dictionary>();
	vars_after->Set("state", GetState());
	vars_after->Set("state_type", GetStateType());
	vars_after->Set("attempt", GetCurrentCheckAttempt());
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
