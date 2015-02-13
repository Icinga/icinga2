/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/icingaapplication.hpp"
#include "icinga/cib.hpp"
#include "base/dynamictype.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/debug.hpp"
#include "base/utility.hpp"
#include "base/timer.hpp"
#include "base/scriptglobal.hpp"
#include "base/initialize.hpp"
#include "base/statsfunction.hpp"

using namespace icinga;

static Timer::Ptr l_RetentionTimer;

REGISTER_TYPE(IcingaApplication);
INITIALIZE_ONCE(&IcingaApplication::StaticInitialize);

void IcingaApplication::StaticInitialize(void)
{
	ScriptGlobal::Set("EnableNotifications", true);
	ScriptGlobal::Set("EnableEventHandlers", true);
	ScriptGlobal::Set("EnableFlapping", true);
	ScriptGlobal::Set("EnableHostChecks", true);
	ScriptGlobal::Set("EnableServiceChecks", true);
	ScriptGlobal::Set("EnablePerfdata", true);

	String node_name = Utility::GetFQDN();

	if (node_name.IsEmpty()) {
		Log(LogNotice, "IcingaApplication", "No FQDN available. Trying Hostname.");
		node_name = Utility::GetHostName();

		if (node_name.IsEmpty()) {
			Log(LogWarning, "IcingaApplication", "No FQDN nor Hostname available. Setting Nodename to 'localhost'.");
			node_name = "localhost";
		}
	}

	ScriptGlobal::Set("NodeName", node_name);
}

REGISTER_STATSFUNCTION(IcingaApplicationStats, &IcingaApplication::StatsFunc);

void IcingaApplication::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	BOOST_FOREACH(const IcingaApplication::Ptr& icingaapplication, DynamicType::GetObjectsByType<IcingaApplication>()) {
		Dictionary::Ptr stats = new Dictionary();
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
}

/**
 * The entry point for the Icinga application.
 *
 * @returns An exit status.
 */
int IcingaApplication::Main(void)
{
	Log(LogDebug, "IcingaApplication", "In IcingaApplication::Main()");

	/* periodically dump the program state */
	l_RetentionTimer = new Timer();
	l_RetentionTimer->SetInterval(300);
	l_RetentionTimer->OnTimerExpired.connect(boost::bind(&IcingaApplication::DumpProgramState, this));
	l_RetentionTimer->Start();

	RunEventLoop();

	Log(LogInformation, "IcingaApplication", "Icinga has shut down.");

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
	return ScriptGlobal::Get("Vars", &Empty);
}

String IcingaApplication::GetNodeName(void) const
{
	return ScriptGlobal::Get("NodeName");
}

bool IcingaApplication::ResolveMacro(const String& macro, const CheckResult::Ptr&, Value *result) const
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
		return ScriptGlobal::Get("EnableNotifications");
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
		return ScriptGlobal::Get("EnableEventHandlers");
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
		return ScriptGlobal::Get("EnableFlapping");
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
		return ScriptGlobal::Get("EnableHostChecks");
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
		return ScriptGlobal::Get("EnableServiceChecks");
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
		return ScriptGlobal::Get("EnablePerfdata");
}

void IcingaApplication::SetEnablePerfdata(bool enabled)
{
	SetOverrideEnablePerfdata(enabled);
}

void IcingaApplication::ClearEnablePerfdata(void)
{
	SetOverrideEnablePerfdata(Empty);
}
