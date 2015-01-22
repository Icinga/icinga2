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

#include "cli/daemoncommand.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "config/configitembuilder.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/logger.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/scriptglobal.hpp"
#include "base/context.hpp"
#include "config.h"
#include <boost/program_options.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

static po::variables_map g_AppParams;

REGISTER_CLICOMMAND("daemon", DaemonCommand);

static String LoadAppType(const String& typeSpec)
{
	Log(LogInformation, "cli")
	    << "Loading application type: " << typeSpec;

	String::SizeType index = typeSpec.FindFirstOf('/');

	if (index == String::NPos)
		return typeSpec;

	String library = typeSpec.SubStr(0, index);

	(void) Utility::LoadExtensionLibrary(library);

	return typeSpec.SubStr(index + 1);
}

static bool ExecuteExpression(Expression *expression)
{
	if (!expression)
		return false;

	try {
		ScriptFrame frame;
		expression->Evaluate(frame);
	} catch (const std::exception& ex) {
		Log(LogCritical, "config", DiagnosticInformation(ex));
		Application::Exit(EXIT_FAILURE);
	}

	return true;
}

static void IncludeZoneDirRecursive(const String& path)
{
	String zoneName = Utility::BaseName(path);

	std::vector<Expression *> expressions;
	Utility::GlobRecursive(path, "*.conf", boost::bind(&ConfigCompiler::CollectIncludes, boost::ref(expressions), _1, zoneName), GlobFile);
	DictExpression expr(expressions);
	ExecuteExpression(&expr);
}

static void IncludeNonLocalZone(const String& zonePath)
{
	String etcPath = Application::GetZonesDir() + "/" + Utility::BaseName(zonePath);

	if (Utility::PathExists(etcPath) || Utility::PathExists(zonePath + "/.authoritative"))
		return;

	IncludeZoneDirRecursive(zonePath);
}

static bool LoadConfigFiles(const boost::program_options::variables_map& vm, const String& appType,
    const String& objectsFile = String(), const String& varsfile = String())
{
	if (!objectsFile.IsEmpty())
		ConfigCompilerContext::GetInstance()->OpenObjectsFile(objectsFile);

	if (vm.count("config") > 0) {
		BOOST_FOREACH(const String& configPath, vm["config"].as<std::vector<std::string> >()) {
			Expression *expression = ConfigCompiler::CompileFile(configPath);
			ExecuteExpression(expression);
			delete expression;
		}
	} else if (!vm.count("no-config")) {
		Expression *expression = ConfigCompiler::CompileFile(Application::GetSysconfDir() + "/icinga2/icinga2.conf");
		ExecuteExpression(expression);
		delete expression;
	}

	/* Load cluster config files - this should probably be in libremote but
	* unfortunately moving it there is somewhat non-trivial. */
	String zonesEtcDir = Application::GetZonesDir();
	if (!zonesEtcDir.IsEmpty() && Utility::PathExists(zonesEtcDir))
		Utility::Glob(zonesEtcDir + "/*", &IncludeZoneDirRecursive, GlobDirectory);

	String zonesVarDir = Application::GetLocalStateDir() + "/lib/icinga2/api/zones";
	if (Utility::PathExists(zonesVarDir))
		Utility::Glob(zonesVarDir + "/*", &IncludeNonLocalZone, GlobDirectory);

	String name, fragment;
	BOOST_FOREACH(boost::tie(name, fragment), ConfigFragmentRegistry::GetInstance()->GetItems()) {
		Expression *expression = ConfigCompiler::CompileText(name, fragment);
		ExecuteExpression(expression);
		delete expression;
	}

	ConfigItemBuilder::Ptr builder = new ConfigItemBuilder();
	builder->SetType(appType);
	builder->SetName("application");
	ConfigItem::Ptr item = builder->Compile();
	item->Register();

	bool result = ConfigItem::CommitItems();

	if (!result)
		return false;

	ConfigCompilerContext::GetInstance()->FinishObjectsFile();

	ScriptGlobal::WriteToFile(varsfile);

	return true;
}

#ifndef _WIN32
static void SigHupHandler(int)
{
	Application::RequestRestart();
}
#endif /* _WIN32 */

static bool Daemonize(void)
{
#ifndef _WIN32
	Application::GetTP().Stop();

	pid_t pid = fork();
	if (pid == -1) {
		return false;
	}

	if (pid) {
		// systemd requires that the pidfile of the daemon is written before the forking
		// process terminates. So wait till either the forked daemon has written a pidfile or died.

		int status;
		int ret;
		pid_t readpid;
		do {
			Utility::Sleep(0.1);

			readpid = Application::ReadPidFile(Application::GetPidPath());
			ret = waitpid(pid, &status, WNOHANG);
		} while (readpid != pid && ret == 0);

		if (ret == pid) {
			Log(LogCritical, "cli", "The daemon could not be started. See log output for details.");
			Application::Exit(EXIT_FAILURE);
		} else if (ret == -1) {
			Log(LogCritical, "cli")
			    << "waitpid() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			Application::Exit(EXIT_FAILURE);
		}

		Application::Exit(0);
	}

	Application::GetTP().Start();
#endif /* _WIN32 */

	return true;
}

static bool SetDaemonIO(const String& stderrFile)
{
#ifndef _WIN32
	int fdnull = open("/dev/null", O_RDWR);
	if (fdnull >= 0) {
		if (fdnull != 0)
			dup2(fdnull, 0);

		if (fdnull != 1)
			dup2(fdnull, 1);

		if (fdnull > 1)
			close(fdnull);
	}

	const char *errPath = "/dev/null";

	if (!stderrFile.IsEmpty())
		errPath = stderrFile.CStr();

	int fderr = open(errPath, O_WRONLY | O_APPEND);

	if (fderr < 0 && errno == ENOENT)
		fderr = open(errPath, O_CREAT | O_WRONLY | O_APPEND, 0600);

	if (fderr > 0) {
		if (fderr != 2)
			dup2(fderr, 2);

		if (fderr > 2)
			close(fderr);
	}

	pid_t sid = setsid();
	if (sid == -1) {
		return false;
	}
#endif

	return true;
}

/**
 * Terminate another process and wait till it has ended
 *
 * @params target PID of the process to end
 */
static void TerminateAndWaitForEnd(pid_t target)
{
#ifndef _WIN32
	// allow 30 seconds timeout
	double timeout = Utility::GetTime() + 30;

	int ret = kill(target, SIGTERM);

	while (Utility::GetTime() < timeout && (ret == 0 || errno != ESRCH)) {
		Utility::Sleep(0.1);
		ret = kill(target, 0);
	}

	// timeout and the process still seems to live: kill it
	if (ret == 0 || errno != ESRCH)
		kill(target, SIGKILL);

#else
	// TODO: implement this for Win32
#endif /* _WIN32 */
}

String DaemonCommand::GetDescription(void) const
{
	return "Starts Icinga 2.";
}

String DaemonCommand::GetShortDescription(void) const
{
	return "starts Icinga 2";
}

void DaemonCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("config,c", po::value<std::vector<std::string> >(), "parse a configuration file")
		("no-config,z", "start without a configuration file")
		("validate,C", "exit after validating the configuration")
		("errorlog,e", po::value<std::string>(), "log fatal errors to the specified log file (only works in combination with --daemonize)")
#ifndef _WIN32
		("daemonize,d", "detach from the controlling terminal")
#endif /* _WIN32 */
	;

#ifndef _WIN32
	hiddenDesc.add_options()
		("reload-internal", po::value<int>(), "used internally to implement config reload: do not call manually, send SIGHUP instead");
#endif /* _WIN32 */
}

std::vector<String> DaemonCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "config" || argument == "errorlog")
		return GetBashCompletionSuggestions("file", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "daemon" CLI command.
 *
 * @returns An exit status.
 */
int DaemonCommand::Run(const po::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!vm.count("validate"))
		Logger::DisableTimestamp(false);

	if (!ScriptGlobal::Exists("UseVfork"))
#ifdef __APPLE__
		ScriptGlobal::Set("UseVfork", false);
#else /* __APPLE__ */
		ScriptGlobal::Set("UseVfork", true);
#endif /* __APPLE__ */

	Log(LogInformation, "cli")
	    << "Icinga application loader (version: " << Application::GetVersion()
#ifdef I2_DEBUG
	    << "; debug"
#endif /* I2_DEBUG */
	    << ")";

	String appType = LoadAppType(Application::GetApplicationType());

	if (!vm.count("validate") && !vm.count("reload-internal")) {
		pid_t runningpid = Application::ReadPidFile(Application::GetPidPath());
		if (runningpid > 0) {
			Log(LogCritical, "cli")
			    << "Another instance of Icinga already running with PID " << runningpid;
			return EXIT_FAILURE;
		}
	}

	if (!LoadConfigFiles(vm, appType, Application::GetObjectsPath(), Application::GetVarsPath()))
		return EXIT_FAILURE;

	if (vm.count("validate")) {
		Log(LogInformation, "cli", "Finished validating the configuration file(s).");
		return EXIT_SUCCESS;
	}

	if(vm.count("reload-internal")) {
		int parentpid = vm["reload-internal"].as<int>();
		Log(LogInformation, "cli")
		    << "Terminating previous instance of Icinga (PID " << parentpid << ")";
		TerminateAndWaitForEnd(parentpid);
		Log(LogInformation, "cli", "Previous instance has ended, taking over now.");
	}

	if (vm.count("daemonize")) {
		if (!vm.count("reload-internal")) {
			// no additional fork neccessary on reload
			try {
				Daemonize();
			} catch (std::exception&) {
				Log(LogCritical, "cli", "Daemonize failed. Exiting.");
				return EXIT_FAILURE;
			}
		}
	}

	// activate config only after daemonization: it starts threads and that is not compatible with fork()
	if (!ConfigItem::ActivateItems()) {
		Log(LogCritical, "cli", "Error activating configuration.");
		return EXIT_FAILURE;
	}

	if (vm.count("daemonize")) {
		String errorLog;
		if (vm.count("errorlog"))
			errorLog = vm["errorlog"].as<std::string>();

		SetDaemonIO(errorLog);
		Logger::DisableConsoleLog();
	}

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &SigHupHandler;
	sigaction(SIGHUP, &sa, NULL);
#endif /* _WIN32 */

	return Application::GetInstance()->Run();
}
