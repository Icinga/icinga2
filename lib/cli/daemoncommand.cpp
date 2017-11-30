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

#include "cli/daemoncommand.hpp"
#include "cli/daemonutility.hpp"
#include "remote/apilistener.hpp"
#include "remote/configobjectutility.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "config/configitembuilder.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/scriptglobal.hpp"
#include "base/context.hpp"
#include "config.h"
#include <boost/program_options.hpp>
#include <boost/tuple/tuple.hpp>
#include <iostream>
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

static po::variables_map g_AppParams;

REGISTER_CLICOMMAND("daemon", DaemonCommand);

#ifndef _WIN32
static void SigHupHandler(int)
{
	Application::RequestRestart();
}
#endif /* _WIN32 */

static bool Daemonize(void)
{
#ifndef _WIN32
	Application::UninitializeBase();

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
			_exit(EXIT_FAILURE);
		} else if (ret == -1) {
			Log(LogCritical, "cli")
			    << "waitpid() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			_exit(EXIT_FAILURE);
		}

		_exit(EXIT_SUCCESS);
	}

	Application::InitializeBase();
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

	if (fderr >= 0) {
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

	// timeout and the process still seems to live: update pid and kill it
	if (ret == 0 || errno != ESRCH) {
		String pidFile = Application::GetPidPath();
		std::ofstream fp(pidFile.CStr());
		fp << Utility::GetPid();
		fp.close();

		kill(target, SIGKILL);
	}

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

	Log(LogInformation, "cli")
	    << "Icinga application loader (version: " << Application::GetAppVersion()
#ifdef I2_DEBUG
	    << "; debug"
#endif /* I2_DEBUG */
	    << ")";

	if (!vm.count("validate") && !vm.count("reload-internal")) {
		pid_t runningpid = Application::ReadPidFile(Application::GetPidPath());
		if (runningpid > 0) {
			Log(LogCritical, "cli")
			    << "Another instance of Icinga already running with PID " << runningpid;
			return EXIT_FAILURE;
		}
	}

	std::vector<std::string> configs;
	if (vm.count("config") > 0)
		configs = vm["config"].as<std::vector<std::string> >();
	else if (!vm.count("no-config"))
		configs.push_back(Application::GetSysconfDir() + "/icinga2/icinga2.conf");

	Log(LogInformation, "cli", "Loading configuration file(s).");

	std::vector<ConfigItem::Ptr> newItems;

	if (!DaemonUtility::LoadConfigFiles(configs, newItems, Application::GetObjectsPath(), Application::GetVarsPath()))
		return EXIT_FAILURE;

	if (vm.count("validate")) {
		Log(LogInformation, "cli", "Finished validating the configuration file(s).");
		return EXIT_SUCCESS;
	}

	if (vm.count("reload-internal")) {
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

	/* restore the previous program state */
	try {
		ConfigObject::RestoreObjects(Application::GetStatePath());
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
		    << "Failed to restore state file: " << DiagnosticInformation(ex);
		return EXIT_FAILURE;
	}

	{
		WorkQueue upq(25000, Application::GetConcurrency());
		upq.SetName("DaemonCommand::Run");

		// activate config only after daemonization: it starts threads and that is not compatible with fork()
		if (!ConfigItem::ActivateItems(upq, newItems, false, false, true)) {
			Log(LogCritical, "cli", "Error activating configuration.");
			return EXIT_FAILURE;
		}
	}

	if (vm.count("daemonize")) {
		String errorLog;
		if (vm.count("errorlog"))
			errorLog = vm["errorlog"].as<std::string>();

		SetDaemonIO(errorLog);
		Logger::DisableConsoleLog();
	}

	/* Remove ignored Downtime/Comment objects. */
	ConfigItem::RemoveIgnoredItems(ConfigObjectUtility::GetConfigDir());

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &SigHupHandler;
	sigaction(SIGHUP, &sa, NULL);
#endif /* _WIN32 */

	ApiListener::UpdateObjectAuthority();

	return Application::GetInstance()->Run();
}
