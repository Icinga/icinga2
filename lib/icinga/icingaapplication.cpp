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

#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/debug.h"
#include "base/utility.h"
#include "base/timer.h"
#include "base/scriptvariable.h"
#include "base/initialize.h"
#include "base/statsfunction.h"

using namespace icinga;

static Timer::Ptr l_RetentionTimer;

REGISTER_TYPE(IcingaApplication);
INITIALIZE_ONCE(&IcingaApplication::StaticInitialize);

void IcingaApplication::StaticInitialize(void)
{
	ScriptVariable::Set("EnableNotifications", true);
	ScriptVariable::Set("EnableEventHandlers", true);
	ScriptVariable::Set("EnableFlapping", true);
	ScriptVariable::Set("EnableHostChecks", true);
	ScriptVariable::Set("EnableServiceChecks", true);
	ScriptVariable::Set("EnablePerfdata", true);
	ScriptVariable::Set("NodeName", Utility::GetHostName());
}

REGISTER_STATSFUNCTION(IcingaApplicationStats, &IcingaApplication::StatsFunc);

Value IcingaApplication::StatsFunc(Dictionary::Ptr& status, Dictionary::Ptr& perfdata)
{
	Dictionary::Ptr nodes = make_shared<Dictionary>();

	BOOST_FOREACH(const IcingaApplication::Ptr& icingaapplication, DynamicType::GetObjects<IcingaApplication>()) {
		Dictionary::Ptr stats = make_shared<Dictionary>();
		stats->Set("node_name", icingaapplication->GetNodeName());
		stats->Set("enable_notifications", icingaapplication->GetEnableNotifications());
		stats->Set("enable_event_handlers", icingaapplication->GetEnableEventHandlers());
		stats->Set("enable_flapping", icingaapplication->GetEnableFlapping());
		stats->Set("enable_host_checks", icingaapplication->GetEnableHostChecks());
		stats->Set("enable_service_checks", icingaapplication->GetEnableServiceChecks());
		stats->Set("enable_perfdata", icingaapplication->GetEnablePerfdata());
		stats->Set("pid", Utility::GetPid());
		stats->Set("program_start", Application::GetStartTime());
		stats->Set("version", Application::GetVersion());

		nodes->Set(icingaapplication->GetName(), stats);
	}

	status->Set("icingaapplication", nodes);

	return 0;
}

/**
 * The entry point for the Icinga application.
 *
 * @returns An exit status.
 */
int IcingaApplication::Main(void)
{
	Log(LogDebug, "icinga", "In IcingaApplication::Main()");

	/* periodically dump the program state */
	l_RetentionTimer = make_shared<Timer>();
	l_RetentionTimer->SetInterval(300);
	l_RetentionTimer->OnTimerExpired.connect(boost::bind(&IcingaApplication::DumpProgramState, this));
	l_RetentionTimer->Start();

	RunEventLoop();

	Log(LogInformation, "icinga", "Icinga has shut down.");

	return EXIT_SUCCESS;
}

void IcingaApplication::OnShutdown(void)
{
	ASSERT(!OwnsLock());

	{
		ObjectLock olock(this);
		l_RetentionTimer->Stop();
	}

	DumpProgramState();
}

void IcingaApplication::DumpProgramState(void)
{
	DynamicObject::DumpObjects(GetStatePath());
}

IcingaApplication::Ptr IcingaApplication::GetInstance(void)
{
	return static_pointer_cast<IcingaApplication>(Application::GetInstance());
}

Dictionary::Ptr IcingaApplication::GetVars(void) const
{
	ScriptVariable::Ptr sv = ScriptVariable::GetByName("Vars");

	if (!sv)
		return Dictionary::Ptr();

	return sv->GetData();
}

String IcingaApplication::GetNodeName(void) const
{
	return ScriptVariable::Get("NodeName");
}

bool IcingaApplication::ResolveMacro(const String& macro, const CheckResult::Ptr&, String *result) const
{
	double now = Utility::GetTime();

	if (macro == "timet") {
		*result = Convert::ToString((long)now);
		return true;
	} else if (macro == "long_date_time") {
		*result = Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", now);
		return true;
	} else if (macro == "short_date_time") {
		*result = Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", now);
		return true;
	} else if (macro == "date") {
		*result = Utility::FormatDateTime("%Y-%m-%d", now);
		return true;
	} else if (macro == "time") {
		*result = Utility::FormatDateTime("%H:%M:%S %z", now);
		return true;
	} else if (macro == "uptime") {
		*result = Utility::FormatDuration(Utility::GetTime() - Application::GetStartTime());
		return true;
	}

	Dictionary::Ptr vars = GetVars();

	if (vars && vars->Contains(macro)) {
		*result = vars->Get(macro);
		return true;
	}

	if (macro.Contains("num_services")) {
		ServiceStatistics ss = CIB::CalculateServiceStats();

		if (macro == "num_services_ok") {
			*result = Convert::ToString(ss.services_ok);
			return true;
		} else if (macro == "num_services_warning") {
			*result = Convert::ToString(ss.services_warning);
			return true;
		} else if (macro == "num_services_critical") {
			*result = Convert::ToString(ss.services_critical);
			return true;
		} else if (macro == "num_services_unknown") {
			*result = Convert::ToString(ss.services_unknown);
			return true;
		} else if (macro == "num_services_pending") {
			*result = Convert::ToString(ss.services_pending);
			return true;
		} else if (macro == "num_services_unreachable") {
			*result = Convert::ToString(ss.services_unreachable);
			return true;
		} else if (macro == "num_services_flapping") {
			*result = Convert::ToString(ss.services_flapping);
			return true;
		} else if (macro == "num_services_in_downtime") {
			*result = Convert::ToString(ss.services_in_downtime);
			return true;
		} else if (macro == "num_services_acknowledged") {
			*result = Convert::ToString(ss.services_acknowledged);
			return true;
		}
	}
	else if (macro.Contains("num_hosts")) {
		HostStatistics hs = CIB::CalculateHostStats();

		if (macro == "num_hosts_up") {
			*result = Convert::ToString(hs.hosts_up);
			return true;
		} else if (macro == "num_hosts_down") {
			*result = Convert::ToString(hs.hosts_down);
			return true;
		} else if (macro == "num_hosts_unreachable") {
			*result = Convert::ToString(hs.hosts_unreachable);
			return true;
		} else if (macro == "num_hosts_flapping") {
			*result = Convert::ToString(hs.hosts_flapping);
			return true;
		} else if (macro == "num_hosts_in_downtime") {
			*result = Convert::ToString(hs.hosts_in_downtime);
			return true;
		} else if (macro == "num_hosts_acknowledged") {
			*result = Convert::ToString(hs.hosts_acknowledged);
			return true;
		}
	}

	return false;
}

bool IcingaApplication::GetEnableNotifications(void) const
{
	if (!GetOverrideEnableNotifications().IsEmpty())
		return GetOverrideEnableNotifications();
	else
		return ScriptVariable::Get("EnableNotifications");
}

void IcingaApplication::SetEnableNotifications(bool enabled)
{
	SetOverrideEnableNotifications(enabled);
}

void IcingaApplication::ClearEnableNotifications(void)
{
	SetOverrideEnableNotifications(Empty);
}

bool IcingaApplication::GetEnableEventHandlers(void) const
{
	if (!GetOverrideEnableEventHandlers().IsEmpty())
		return GetOverrideEnableEventHandlers();
	else
		return ScriptVariable::Get("EnableEventHandlers");
}

void IcingaApplication::SetEnableEventHandlers(bool enabled)
{
	SetOverrideEnableEventHandlers(enabled);
}

void IcingaApplication::ClearEnableEventHandlers(void)
{
	SetOverrideEnableEventHandlers(Empty);
}

bool IcingaApplication::GetEnableFlapping(void) const
{
	if (!GetOverrideEnableFlapping().IsEmpty())
		return GetOverrideEnableFlapping();
	else
		return ScriptVariable::Get("EnableFlapping");
}

void IcingaApplication::SetEnableFlapping(bool enabled)
{
	SetOverrideEnableFlapping(enabled);
}

void IcingaApplication::ClearEnableFlapping(void)
{
	SetOverrideEnableFlapping(Empty);
}

bool IcingaApplication::GetEnableHostChecks(void) const
{
	if (!GetOverrideEnableHostChecks().IsEmpty())
		return GetOverrideEnableHostChecks();
	else
		return ScriptVariable::Get("EnableHostChecks");
}

void IcingaApplication::SetEnableHostChecks(bool enabled)
{
	SetOverrideEnableHostChecks(enabled);
}

void IcingaApplication::ClearEnableHostChecks(void)
{
	SetOverrideEnableHostChecks(Empty);
}

bool IcingaApplication::GetEnableServiceChecks(void) const
{
	if (!GetOverrideEnableServiceChecks().IsEmpty())
		return GetOverrideEnableServiceChecks();
	else
		return ScriptVariable::Get("EnableServiceChecks");
}

void IcingaApplication::SetEnableServiceChecks(bool enabled)
{
	SetOverrideEnableServiceChecks(enabled);
}

void IcingaApplication::ClearEnableServiceChecks(void)
{
	SetOverrideEnableServiceChecks(Empty);
}

bool IcingaApplication::GetEnablePerfdata(void) const
{
	if (!GetOverrideEnablePerfdata().IsEmpty())
		return GetOverrideEnablePerfdata();
	else
		return ScriptVariable::Get("EnablePerfdata");
}

void IcingaApplication::SetEnablePerfdata(bool enabled)
{
	SetOverrideEnablePerfdata(enabled);
}

void IcingaApplication::ClearEnablePerfdata(void)
{
	SetOverrideEnablePerfdata(Empty);
}
