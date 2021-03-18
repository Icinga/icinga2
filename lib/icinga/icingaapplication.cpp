/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/icingaapplication.hpp"
#include "icinga/icingaapplication-ti.cpp"
#include "icinga/cib.hpp"
#include "icinga/macroprocessor.hpp"
#include "config/configcompiler.hpp"
#include "base/configwriter.hpp"
#include "base/configtype.hpp"
#include "base/exception.hpp"
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
#include <fstream>

using namespace icinga;

static Timer::Ptr l_RetentionTimer;

REGISTER_TYPE(IcingaApplication);
/* Ensure that the priority is lower than the basic System namespace initialization in scriptframe.cpp. */
INITIALIZE_ONCE_WITH_PRIORITY(&IcingaApplication::StaticInitialize, 50);

void IcingaApplication::StaticInitialize()
{
	/* Pre-fill global constants, can be overridden with user input later in icinga-app/icinga.cpp. */
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

	ScriptGlobal::Set("ReloadTimeout", 300);
	ScriptGlobal::Set("MaxConcurrentChecks", 512);

	Namespace::Ptr systemNS = ScriptGlobal::Get("System");
	/* Ensure that the System namespace is already initialized. Otherwise this is a programming error. */
	VERIFY(systemNS);

	systemNS->Set("ApplicationType", "IcingaApplication", true);
	systemNS->Set("ApplicationVersion", Application::GetAppVersion(), true);

	Namespace::Ptr globalNS = ScriptGlobal::GetGlobals();
	VERIFY(globalNS);

	auto icingaNSBehavior = new ConstNamespaceBehavior();
	icingaNSBehavior->Freeze();
	Namespace::Ptr icingaNS = new Namespace(icingaNSBehavior);
	globalNS->SetAttribute("Icinga", new ConstEmbeddedNamespaceValue(icingaNS));
}

REGISTER_STATSFUNCTION(IcingaApplication, &IcingaApplication::StatsFunc);

void IcingaApplication::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData nodes;

	for (const IcingaApplication::Ptr& icingaapplication : ConfigType::GetObjectsByType<IcingaApplication>()) {
		nodes.emplace_back(icingaapplication->GetName(), new Dictionary({
			{ "node_name", icingaapplication->GetNodeName() },
			{ "enable_notifications", icingaapplication->GetEnableNotifications() },
			{ "enable_event_handlers", icingaapplication->GetEnableEventHandlers() },
			{ "enable_flapping", icingaapplication->GetEnableFlapping() },
			{ "enable_host_checks", icingaapplication->GetEnableHostChecks() },
			{ "enable_service_checks", icingaapplication->GetEnableServiceChecks() },
			{ "enable_perfdata", icingaapplication->GetEnablePerfdata() },
			{ "environment", icingaapplication->GetEnvironment() },
			{ "pid", Utility::GetPid() },
			{ "program_start", Application::GetStartTime() },
			{ "version", Application::GetAppVersion() }
		}));
	}

	status->Set("icingaapplication", new Dictionary(std::move(nodes)));
}

/**
 * The entry point for the Icinga application.
 *
 * @returns An exit status.
 */
int IcingaApplication::Main()
{
	Log(LogDebug, "IcingaApplication", "In IcingaApplication::Main()");

	/* periodically dump the program state */
	l_RetentionTimer = new Timer();
	l_RetentionTimer->SetInterval(300);
	l_RetentionTimer->OnTimerExpired.connect([this](const Timer * const&) { DumpProgramState(); });
	l_RetentionTimer->Start();

	RunEventLoop();

	Log(LogInformation, "IcingaApplication", "Icinga has shut down.");

	return EXIT_SUCCESS;
}

void IcingaApplication::OnShutdown()
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

		Array::Ptr args1 = new Array({
			object->GetReflectionType()->GetName(),
			object->GetName()
		});
		ConfigWriter::EmitFunctionCall(fp, "get_object", args1);

		ConfigWriter::EmitRaw(fp, "\nif (obj) {\n");
	}

	ConfigWriter::EmitRaw(fp, "\tobj.");

	Array::Ptr args2 = new Array({
		attr,
		value
	});
	ConfigWriter::EmitFunctionCall(fp, "modify_attribute", args2);

	ConfigWriter::EmitRaw(fp, "\n");

	previousObject = object;
}

void IcingaApplication::DumpProgramState()
{
	ConfigObject::DumpObjects(Configuration::StatePath);
	DumpModifiedAttributes();
}

void IcingaApplication::DumpModifiedAttributes()
{
	String path = Configuration::ModAttrPath;

	try {
		Utility::Glob(path + ".tmp.*", &Utility::Remove, GlobFile);
	} catch (const std::exception& ex) {
		Log(LogWarning, "IcingaApplication") << DiagnosticInformation(ex);
	}

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(path + ".tmp.XXXXXX", 0644, fp);
	fp.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	ConfigObject::Ptr previousObject;
	ConfigObject::DumpModifiedAttributes([&fp, &previousObject](const ConfigObject::Ptr& object, const String& attr, const Value& value) {
		PersistModAttrHelper(fp, previousObject, object, attr, value);
	});

	if (previousObject) {
		ConfigWriter::EmitRaw(fp, "\tobj.version = ");
		ConfigWriter::EmitValue(fp, 0, previousObject->GetVersion());
		ConfigWriter::EmitRaw(fp, "\n}\n");
	}

	fp.close();

	Utility::RenameFile(tempFilename, path);
}

IcingaApplication::Ptr IcingaApplication::GetInstance()
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
		*result = Utility::FormatDuration(Application::GetUptime());
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
		} else if (macro == "num_services_handled") {
			*result = ss.services_handled;
			return true;
		} else if (macro == "num_services_problem") {
			*result = ss.services_problem;
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
		} else if (macro == "num_hosts_handled") {
			*result = hs.hosts_handled;
			return true;
		} else if (macro == "num_hosts_problem") {
			*result = hs.hosts_problem;
			return true;
		}
	}

	return false;
}

String IcingaApplication::GetNodeName() const
{
	return ScriptGlobal::Get("NodeName");
}

/* Intentionally kept here, since an agent may not have the CheckerComponent loaded. */
int IcingaApplication::GetMaxConcurrentChecks() const
{
	return ScriptGlobal::Get("MaxConcurrentChecks");
}

String IcingaApplication::GetEnvironment() const
{
	return Application::GetAppEnvironment();
}

void IcingaApplication::SetEnvironment(const String& value, bool suppress_events, const Value& cookie)
{
	Application::SetAppEnvironment(value);
}

void IcingaApplication::ValidateVars(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
	MacroProcessor::ValidateCustomVars(this, lvalue());
}
