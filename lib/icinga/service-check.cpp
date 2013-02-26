/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-icinga.h"

using namespace icinga;

const int Service::DefaultMaxCheckAttempts = 3;
const int Service::DefaultCheckInterval = 5 * 60;
const int Service::CheckIntervalDivisor = 5;

signals2::signal<void (const Service::Ptr&, const String&)> Service::OnCheckerChanged;
signals2::signal<void (const Service::Ptr&, const Value&)> Service::OnNextCheckChanged;

Value Service::GetCheckCommand(void) const
{
	return m_CheckCommand;
}

long Service::GetMaxCheckAttempts(void) const
{
	if (m_MaxCheckAttempts.IsEmpty())
		return DefaultMaxCheckAttempts;

	return m_MaxCheckAttempts;
}

double Service::GetCheckInterval(void) const
{
	if (m_CheckInterval.IsEmpty())
		return DefaultCheckInterval;

	return m_CheckInterval;
}

double Service::GetRetryInterval(void) const
{
	if (m_RetryInterval.IsEmpty())
		return GetCheckInterval() / CheckIntervalDivisor;

	return m_RetryInterval;
}

Dictionary::Ptr Service::GetCheckers(void) const
{
	return m_Checkers;
}

void Service::SetSchedulingOffset(long offset)
{
	m_SchedulingOffset = offset;
}

long Service::GetSchedulingOffset(void)
{
	return m_SchedulingOffset;
}

void Service::SetNextCheck(double nextCheck)
{
	m_NextCheck = nextCheck;
	Touch("next_check");
}

double Service::GetNextCheck(void)
{
	if (m_NextCheck.IsEmpty()) {
		UpdateNextCheck();

		if (m_NextCheck.IsEmpty())
			BOOST_THROW_EXCEPTION(runtime_error("Failed to schedule next check."));
	}

	return m_NextCheck;
}

void Service::UpdateNextCheck(void)
{
	double interval;

	if (GetStateType() == StateTypeSoft)
		interval = GetRetryInterval();
	else
		interval = GetCheckInterval();

	double now = Utility::GetTime();
	double adj = 0;

	if (interval > 1)
		adj = fmod(now + GetSchedulingOffset(), interval);

	SetNextCheck(now - adj + interval);
}

void Service::SetCurrentChecker(const String& checker)
{
	m_CurrentChecker = checker;
	Touch("current_checker");
}

String Service::GetCurrentChecker(void) const
{
	return m_CurrentChecker;
}

void Service::SetCurrentCheckAttempt(long attempt)
{
	m_CheckAttempt = attempt;
	Touch("check_attempt");
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
	Touch("state");
}

ServiceState Service::GetState(void) const
{
	if (m_State.IsEmpty())
		return StateUnknown;

	int ivalue = static_cast<int>(m_State);
	return static_cast<ServiceState>(ivalue);
}

void Service::SetStateType(ServiceStateType type)
{
	m_StateType = static_cast<long>(type);
	Touch("state_type");
}

ServiceStateType Service::GetStateType(void) const
{
	if (m_StateType.IsEmpty())
		return StateTypeSoft;

	int ivalue = static_cast<int>(m_StateType);
	return static_cast<ServiceStateType>(ivalue);
}

void Service::SetLastCheckResult(const Dictionary::Ptr& result)
{
	m_LastResult = result;
	Touch("last_result");
}

Dictionary::Ptr Service::GetLastCheckResult(void) const
{
	return m_LastResult;
}

void Service::SetLastStateChange(double ts)
{
	m_LastStateChange = ts;
	Touch("last_state_change");
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
	Touch("last_hard_state_change");
}

double Service::GetLastHardStateChange(void) const
{
	if (m_LastHardStateChange.IsEmpty())
		return IcingaApplication::GetInstance()->GetStartTime();

	return m_LastHardStateChange;
}

bool Service::GetEnableActiveChecks(void) const
{
	if (m_EnableActiveChecks.IsEmpty())
		return true;

	return static_cast<bool>(m_EnableActiveChecks);
}

void Service::SetEnableActiveChecks(bool enabled)
{
	m_EnableActiveChecks = enabled ? 1 : 0;
	Touch("enable_active_checks");
}

bool Service::GetEnablePassiveChecks(void) const
{
	if (m_EnablePassiveChecks.IsEmpty())
		return true;

	return static_cast<bool>(m_EnablePassiveChecks);
}

void Service::SetEnablePassiveChecks(bool enabled)
{
	m_EnablePassiveChecks = enabled ? 1 : 0;
	Touch("enable_passive_checks");
}

bool Service::GetForceNextCheck(void) const
{
	if (m_ForceNextCheck.IsEmpty())
		return false;

	return static_cast<bool>(m_ForceNextCheck);
}

void Service::SetForceNextCheck(bool forced)
{
	m_ForceNextCheck = forced ? 1 : 0;
	Touch("force_next_check");
}

void Service::ApplyCheckResult(const Dictionary::Ptr& cr)
{
	ServiceState old_state = GetState();
	ServiceStateType old_stateType = GetStateType();
	bool hardChange = false;
	bool recovery;

	long attempt = GetCurrentCheckAttempt();

	if (cr->Get("state") == StateOK) {
		if (old_state != StateOK && old_stateType == StateTypeHard)
			hardChange = true; // hard recovery

		if (old_state == StateOK)
			SetStateType(StateTypeHard);

		attempt = 1;
		recovery = true;
	} else {
		if (attempt >= GetMaxCheckAttempts()) {
			SetStateType(StateTypeHard);
			attempt = 1;
			hardChange = true;
		} else if (GetStateType() == StateTypeSoft || GetState() == StateOK) {
			SetStateType(StateTypeSoft);
			attempt++;
		}

		recovery = false;
	}

	SetCurrentCheckAttempt(attempt);

	int state = cr->Get("state");
	SetState(static_cast<ServiceState>(state));

	SetLastCheckResult(cr);

	double now = Utility::GetTime();

	if (old_state != GetState()) {
		SetLastStateChange(now);

		/* remove acknowledgements */
		if (GetAcknowledgement() == AcknowledgementNormal ||
		    (GetAcknowledgement() == AcknowledgementSticky && GetStateType() == StateTypeHard && GetState() == StateOK)) {
			SetAcknowledgement(AcknowledgementNone);
			SetAcknowledgementExpiry(0);
		}

		/* reschedule service dependencies */
		BOOST_FOREACH(const Service::Ptr& parent, GetParentServices()) {
			parent->SetNextCheck(Utility::GetTime());
		}

		/* reschedule host dependencies */
		BOOST_FOREACH(const Host::Ptr& parent, GetParentHosts()) {
			Service::Ptr service = parent->GetHostCheckService();

			if (service)
				service->SetNextCheck(Utility::GetTime());
		}
	}

	if (GetState() != StateOK)
		TriggerDowntimes();

	if (hardChange) {
		SetLastHardStateChange(now);

		/* Make sure the notification component sees the updated
		 * state/state_type attributes. */
		Flush();

		if (IsReachable(GetSelf()) && !IsInDowntime() && !IsAcknowledged())
			RequestNotifications(recovery ? NotificationRecovery : NotificationProblem);
	}
}

ServiceState Service::StateFromString(const String& state)
{
	if (state == "OK")
		return StateOK;
	else if (state == "WARNING")
		return StateWarning;
	else if (state == "CRITICAL")
		return StateCritical;
	else if (state == "UNCHECKABLE")
		return StateUncheckable;
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
		case StateUncheckable:
			return "UNCHECKABLE";
		case StateUnknown:
		default:
			return "UNKNOWN";
	}
}

ServiceStateType Service::StateTypeFromString(const String& type)
{
	if (type == "SOFT")
		return StateTypeSoft;
	else
		return StateTypeHard;
}

String Service::StateTypeToString(ServiceStateType type)
{
	if (type == StateTypeSoft)
		return "SOFT";
	else
		return "HARD";
}

bool Service::IsAllowedChecker(const String& checker) const
{
	Dictionary::Ptr checkers = GetCheckers();

	if (!checkers)
		return true;

	Value pattern;
	BOOST_FOREACH(tie(tuples::ignore, pattern), checkers) {
		if (Utility::Match(pattern, checker))
			return true;
	}

	return false;
}

void Service::BeginExecuteCheck(const Service::Ptr& self, const function<void (void)>& callback)
{
	ObjectLock slock(self);

	/* don't run another check if there is one pending */
	if (self->m_CurrentTask) {
		slock.Unlock();

		/* we need to call the callback anyway */
		callback();

		return;
	}

	/* keep track of scheduling info in case the check type doesn't provide its own information */
	Dictionary::Ptr checkInfo = boost::make_shared<Dictionary>();
	checkInfo->Set("schedule_start", self->GetNextCheck());
	checkInfo->Set("execution_start", Utility::GetTime());

	vector<Dictionary::Ptr> macroDicts;
	macroDicts.push_back(self->GetMacros());
	macroDicts.push_back(Service::CalculateDynamicMacros(self));

	Value raw_command = self->GetCheckCommand();

	Host::Ptr host = self->GetHost();

	slock.Unlock();

	{
		ObjectLock olock(host);
		macroDicts.push_back(host->GetMacros());
		macroDicts.push_back(Host::CalculateDynamicMacros(host));
	}

	IcingaApplication::Ptr app = IcingaApplication::GetInstance();

	{
		ObjectLock olock(app);
		macroDicts.push_back(app->GetMacros());
	}

	macroDicts.push_back(IcingaApplication::CalculateDynamicMacros(app));

	Dictionary::Ptr macros = MacroProcessor::MergeMacroDicts(macroDicts);

	checkInfo->Set("macros", macros);

	vector<Value> arguments;
	arguments.push_back(self);
	arguments.push_back(macros);

	ScriptTask::Ptr task;

	{
		ObjectLock olock(self);
		task = self->MakeMethodTask("check", arguments);
		self->m_CurrentTask = task;
	}

	task->Start(boost::bind(&Service::CheckCompletedHandler, self, checkInfo, _1, callback));
}

void Service::CheckCompletedHandler(const Dictionary::Ptr& checkInfo,
    const ScriptTask::Ptr& task, const function<void (void)>& callback)
{
	checkInfo->Set("execution_end", Utility::GetTime());
	checkInfo->Set("schedule_end", Utility::GetTime());

	Dictionary::Ptr result;

	try {
		Value vresult;

		{
			ObjectLock tlock(task);
			vresult = task->GetResult();
		}

		if (vresult.IsObjectType<Dictionary>())
			result = vresult;
	} catch (const exception& ex) {
		stringstream msgbuf;
		msgbuf << "Exception occured during check for service '"
		       << GetName() << "': " << diagnostic_information(ex);
		String message = msgbuf.str();

		Logger::Write(LogWarning, "icinga", message);

		result = boost::make_shared<Dictionary>();
		result->Set("state", StateUnknown);
		result->Set("output", message);
	}

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

		if (!result->Contains("current_checker")) {
			EndpointManager::Ptr em = EndpointManager::GetInstance();
			ObjectLock olock(em);

			result->Set("current_checker", em->GetIdentity());
		}
	}

	{
		ObjectLock olock(this);
		if (result)
			ProcessCheckResult(result);

		m_CurrentTask.reset();

		/* figure out when the next check is for this service; the call to
		 * ApplyCheckResult() should've already done this but lets do it again
		 * just in case there was no check result. */
		UpdateNextCheck();
	}

	callback();
}

void Service::ProcessCheckResult(const Dictionary::Ptr& cr)
{
	ApplyCheckResult(cr);

	Service::UpdateStatistics(cr);

	/* Flush the object so other instances see the service's
	 * new state when they receive the CheckResult message */
	Flush();

	RequestMessage rm;
	rm.SetMethod("checker::CheckResult");

	/* TODO: add _old_ state to message */
	CheckResultMessage params;
	params.SetService(GetName());
	params.SetCheckResult(cr);

	rm.SetParams(params);

	EndpointManager::Ptr em = EndpointManager::GetInstance();
	ObjectLock olock(em);
	em->SendMulticastMessage(rm);
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
	ObjectLock olock(cr);

	double execution_start = 0, execution_end = 0;

	if (cr) {
		ObjectLock olock(cr);

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
		ObjectLock olock(cr);

		if (!cr->Contains("schedule_start") || !cr->Contains("schedule_end"))
			return 0;

		schedule_start = cr->Get("schedule_start");
		schedule_end = cr->Get("schedule_end");
	}

	return (schedule_end - schedule_start) - CalculateExecutionTime(cr);

}
