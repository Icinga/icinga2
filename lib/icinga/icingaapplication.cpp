/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include "icinga/icingaapplication.tcpp"
#include "icinga/cib.hpp"
#include "icinga/macroprocessor.hpp"
#include "config/configcompiler.hpp"
#include "base/configwriter.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/debug.hpp"
#include "base/utility.hpp"
#include "base/timer.hpp"
#include "base/scriptglobal.hpp"
#include "base/initialize.hpp"
#include "base/statsfunction.hpp"
#include "base/loader.hpp"

using namespace icinga;

static Timer::Ptr l_RetentionTimer;

REGISTER_TYPE(IcingaApplication);
INITIALIZE_ONCE(&IcingaApplication::StaticInitialize);

void IcingaApplication::StaticInitialize(void)
{
	Loader::LoadExtensionLibrary("methods");

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

	ScriptGlobal::Set("ApplicationType", "IcingaApplication");
}

REGISTER_STATSFUNCTION(IcingaApplication, &IcingaApplication::StatsFunc);

void IcingaApplication::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const IcingaApplication::Ptr& icingaapplication : ConfigType::GetObjectsByType<IcingaApplication>()) {
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
		stats->Set("version", Application::GetAppVersion());

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
	l_RetentionTimer->OnTimerExpired.connect(std::bind(&IcingaApplication::DumpProgramState, this));
	l_RetentionTimer->Start();

	RunEventLoop();

	Log(LogInformation, "IcingaApplication", "Icinga has shut down.");

	return EXIT_SUCCESS;
}

void IcingaApplication::OnShutdown(void)
{
	{
		ObjectLock olock(this);
		l_RetentionTimer->Stop();
	}

	DumpProgramState();
}

static void PersistModAttrHelper(std::fstream& fp, ConfigObject::Ptr& previousObject, const ConfigObject::Ptr& object, const String& attr, const Value& value)
{
	if (object != previousObject) {
		if (previousObject) {
			ConfigWriter::EmitRaw(fp, "\tobj.version = ");
			ConfigWriter::EmitValue(fp, 0, previousObject->GetVersion());
			ConfigWriter::EmitRaw(fp, "\n}\n\n");
		}

		ConfigWriter::EmitRaw(fp, "var obj = ");

		Array::Ptr args1 = new Array();
		args1->Add(object->GetReflectionType()->GetName());
		args1->Add(object->GetName());
		ConfigWriter::EmitFunctionCall(fp, "get_object", args1);

		ConfigWriter::EmitRaw(fp, "\nif (obj) {\n");
	}

	ConfigWriter::EmitRaw(fp, "\tobj.");

	Array::Ptr args2 = new Array();
	args2->Add(attr);
	args2->Add(value);
	ConfigWriter::EmitFunctionCall(fp, "modify_attribute", args2);

	ConfigWriter::EmitRaw(fp, "\n");

	previousObject = object;
}

void IcingaApplication::DumpProgramState(void)
{
	ConfigObject::DumpObjects(GetStatePath());
	DumpModifiedAttributes();
}

void IcingaApplication::DumpModifiedAttributes(void)
{
	String path = GetModAttrPath();

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(path + ".XXXXXX", 0644, fp);
	fp.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	ConfigObject::Ptr previousObject;
	ConfigObject::DumpModifiedAttributes(std::bind(&PersistModAttrHelper, std::ref(fp), std::ref(previousObject), _1, _2, _3));

	if (previousObject) {
		ConfigWriter::EmitRaw(fp, "\tobj.version = ");
		ConfigWriter::EmitValue(fp, 0, previousObject->GetVersion());
		ConfigWriter::EmitRaw(fp, "\n}\n");
	}

	fp.close();

#ifdef _WIN32
	_unlink(path.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), path.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempFilename));
	}
}

IcingaApplication::Ptr IcingaApplication::GetInstance(void)
{
	return static_pointer_cast<IcingaApplication>(Application::GetInstance());
}

bool IcingaApplication::ResolveMacro(const String& macro, const CheckResult::Ptr&, Value *result) const
{
	double now = Utility::GetTime();

	if (macro == "timet") {
		*result = static_cast<long>(now);
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
			*result = ss.services_ok;
			return true;
		} else if (macro == "num_services_warning") {
			*result = ss.services_warning;
			return true;
		} else if (macro == "num_services_critical") {
			*result = ss.services_critical;
			return true;
		} else if (macro == "num_services_unknown") {
			*result = ss.services_unknown;
			return true;
		} else if (macro == "num_services_pending") {
			*result = ss.services_pending;
			return true;
		} else if (macro == "num_services_unreachable") {
			*result = ss.services_unreachable;
			return true;
		} else if (macro == "num_services_flapping") {
			*result = ss.services_flapping;
			return true;
		} else if (macro == "num_services_in_downtime") {
			*result = ss.services_in_downtime;
			return true;
		} else if (macro == "num_services_acknowledged") {
			*result = ss.services_acknowledged;
			return true;
		}
	}
	else if (macro.Contains("num_hosts")) {
		HostStatistics hs = CIB::CalculateHostStats();

		if (macro == "num_hosts_up") {
			*result = hs.hosts_up;
			return true;
		} else if (macro == "num_hosts_down") {
			*result = hs.hosts_down;
			return true;
		} else if (macro == "num_hosts_pending") {
			*result = hs.hosts_pending;
			return true;
		} else if (macro == "num_hosts_unreachable") {
			*result = hs.hosts_unreachable;
			return true;
		} else if (macro == "num_hosts_flapping") {
			*result = hs.hosts_flapping;
			return true;
		} else if (macro == "num_hosts_in_downtime") {
			*result = hs.hosts_in_downtime;
			return true;
		} else if (macro == "num_hosts_acknowledged") {
			*result = hs.hosts_acknowledged;
			return true;
		}
	}

	return false;
}

String IcingaApplication::GetNodeName(void) const
{
	return ScriptGlobal::Get("NodeName");
}

void IcingaApplication::ValidateVars(const Dictionary::Ptr& value, const ValidationUtils& utils)
{
	MacroProcessor::ValidateCustomVars(this, value);
}
