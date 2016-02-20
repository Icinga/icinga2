/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include <boost/foreach.hpp>

using namespace icinga;

boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, const MessageOrigin::Ptr&)> Checkable::OnNewCheckResult;
boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, StateType, const MessageOrigin::Ptr&)> Checkable::OnStateChange;
boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, std::set<Checkable::Ptr>, const MessageOrigin::Ptr&)> Checkable::OnReachabilityChanged;
boost::signals2::signal<void (const Checkable::Ptr&, NotificationType, const CheckResult::Ptr&, const String&, const String&)> Checkable::OnNotificationsRequested;
boost::signals2::signal<void (const Checkable::Ptr&)> Checkable::OnNextCheckUpdated;

CheckCommand::Ptr Checkable::GetCheckCommand(void) const
{
	return dynamic_pointer_cast<CheckCommand>(NavigateCheckCommandRaw());
}

TimePeriod::Ptr Checkable::GetCheckPeriod(void) const
{
	return TimePeriod::GetByName(GetCheckPeriodRaw());
}

void Checkable::SetSchedulingOffset(long offset)
{
	m_SchedulingOffset = offset;
}

long Checkable::GetSchedulingOffset(void)
{
	return m_SchedulingOffset;
}

void Checkable::UpdateNextCheck(void)
{
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

void Checkable::ProcessCheckResult(const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin)
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

	if (old_cr && cr->GetExecutionStart() < old_cr->GetExecutionStart())
		return;

	/* The ExecuteCheck function already sets the old state, but we need to do it again
	 * in case this was a passive check result. */
	SetLastStateRaw(old_state);
	SetLastStateType(old_stateType);
	SetLastReachable(reachable);

	long attempt = 1;

	std::set<Checkable::Ptr> children = GetChildren();

	if (!old_cr) {
		SetStateType(StateTypeHard);
	} else if (cr->GetState() == ServiceOK) {
		if (old_state == ServiceOK && old_stateType == StateTypeSoft) {
			SetStateType(StateTypeHard); // SOFT OK -> HARD OK
			recovery = true;
		}

		if (old_state != ServiceOK)
			recovery = true; // NOT OK -> SOFT/HARD OK

		ResetNotificationNumbers();
		SaveLastState(ServiceOK, Utility::GetTime());

		/* update reachability for child objects in OK state */
		if (!children.empty())
			OnReachabilityChanged(this, cr, children, origin);
	} else {
		if (old_attempt >= GetMaxCheckAttempts()) {
			SetStateType(StateTypeHard);
		} else if (old_stateType == StateTypeSoft && old_state != ServiceOK) {
			SetStateType(StateTypeSoft);
			attempt = old_attempt + 1; //NOT-OK -> NOT-OK counter
		} else if (old_state == ServiceOK) {
			SetStateType(StateTypeSoft);
			attempt = 1; //OK -> NOT-OK transition, reset the counter
		} else {
			attempt = old_attempt;
		}

		if (cr->GetState() != ServiceOK) {
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

	bool is_volatile = GetVolatile();

	if (hardChange || is_volatile) {
		SetLastHardStateRaw(new_state);
		SetLastHardStateChange(now);
	}

	if (new_state != ServiceOK)
		TriggerDowntimes();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(this);

	CheckableType checkable_type = CheckableHost;
	if (service)
		checkable_type = CheckableService;

	/* statistics for external tools */
	Checkable::UpdateStatistics(cr, checkable_type);

	bool in_downtime = IsInDowntime();
	bool send_notification = hardChange && notification_reachable && !in_downtime && !IsAcknowledged();

	if (!old_cr)
		send_notification = false; /* Don't send notifications for the initial state change */

	if (old_state == ServiceOK && old_stateType == StateTypeSoft)
		send_notification = false; /* Don't send notifications for SOFT-OK -> HARD-OK. */

	if (is_volatile && old_state == ServiceOK && new_state == ServiceOK)
		send_notification = false; /* Don't send notifications for volatile OK -> OK changes. */

	bool send_downtime_notification = (GetLastInDowntime() != in_downtime);
	SetLastInDowntime(in_downtime);

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

	bool was_flapping, is_flapping;

	was_flapping = IsFlapping();
	if (GetStateType() == StateTypeHard)
		UpdateFlappingStatus(stateChange);
	is_flapping = IsFlapping();

	/* update next check time for active and passive check results */
	if (!GetEnableActiveChecks() && GetEnablePassiveChecks()) {
		/* Reschedule the next passive check. The side effect of this is that for as long
		 * as we receive passive results for a service we won't execute any
		 * active checks. */
		SetNextCheck(Utility::GetTime() + GetCheckInterval());
	} else if (GetEnableActiveChecks()) {
		/* update next check time based on state changes and types */
		UpdateNextCheck();
	}

	olock.Unlock();

//	Log(LogDebug, "Checkable")
//	    << "Flapping: Checkable " << GetName()
//	    << " was: " << (was_flapping)
//	    << " is: " << is_flapping)
//	    << " threshold: " << GetFlappingThreshold()
//	    << "% current: " + GetFlappingCurrent()) << "%.";

	OnNewCheckResult(this, cr, origin);

	/* signal status updates to for example db_ido */
	OnStateChanged(this);

	String old_state_str = (service ? Service::StateToString(old_state) : Host::StateToString(Host::CalculateState(old_state)));
	String new_state_str = (service ? Service::StateToString(new_state) : Host::StateToString(Host::CalculateState(new_state)));

	if (hardChange || is_volatile) {
		OnStateChange(this, cr, StateTypeHard, origin);
		Log(LogNotice, "Checkable")
		    << "State Change: Checkable " << GetName() << " hard state change from " << old_state_str << " to " << new_state_str << " detected." << (is_volatile ? " Checkable is volatile." : "");
	} else if (stateChange) {
		OnStateChange(this, cr, StateTypeSoft, origin);
		Log(LogNotice, "Checkable")
		    << "State Change: Checkable " << GetName() << " soft state change from " << old_state_str << " to " << new_state_str << " detected.";
	}

	if (GetStateType() == StateTypeSoft || hardChange || recovery || is_volatile)
		ExecuteEventHandler();

	if (send_downtime_notification)
		OnNotificationsRequested(this, in_downtime ? NotificationDowntimeStart : NotificationDowntimeEnd, cr, "", "");

	if (!was_flapping && is_flapping) {
		OnNotificationsRequested(this, NotificationFlappingStart, cr, "", "");

		Log(LogNotice, "Checkable")
		    << "Flapping: Checkable " << GetName() << " started flapping (" << GetFlappingThreshold() << "% < " << GetFlappingCurrent() << "%).";

		NotifyFlapping(origin);
	} else if (was_flapping && !is_flapping) {
		OnNotificationsRequested(this, NotificationFlappingEnd, cr, "", "");

		Log(LogNotice, "Checkable")
		    << "Flapping: Checkable " << GetName() << " stopped flapping (" << GetFlappingThreshold() << "% >= " << GetFlappingCurrent() << "%).";

		NotifyFlapping(origin);
	} else if (send_notification)
		OnNotificationsRequested(this, recovery ? NotificationRecovery : NotificationProblem, cr, "", "");
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

void Checkable::ExecuteCheck(void)
{
	CONTEXT("Executing check for object '" + GetName() + "'");

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

	CheckResult::Ptr cr = new CheckResult();

	cr->SetScheduleStart(scheduled_start);
	cr->SetExecutionStart(before_check);

	Endpoint::Ptr endpoint = GetCommandEndpoint();
	bool local = !endpoint || endpoint == Endpoint::GetLocalEndpoint();

	if (local) {
		GetCheckCommand()->Execute(this, cr, NULL, false);
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
			   a check result from the remote instance. The check will be re-scheduled
			   using the proper check interval once we've received a check result. */
			SetNextCheck(Utility::GetTime() + GetCheckCommand()->GetTimeout() + 30);
		} else if (Application::GetInstance()->GetStartTime() < Utility::GetTime() - 300) {
			/* fail to perform check on unconnected endpoint */
			cr->SetState(ServiceUnknown);

			cr->SetOutput("Remote Icinga instance '" + endpoint->GetName() +
			    "' " + "is not connected to '" + Endpoint::GetLocalEndpoint()->GetName() + "'");

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
