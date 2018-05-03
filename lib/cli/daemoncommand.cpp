/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

static bool Daemonize()
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

String DaemonCommand::GetDescription() const
{
	return "Starts Icinga 2.";
}

String DaemonCommand::GetShortDescription() const
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
#else
	hiddenDesc.add_options()
		("restart-service", po::value<std::string>(), "tries to restart the Icinga 2 service");
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
	Logger::EnableTimestamp();

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

#ifdef _WIN32
	if (vm.count("restart-service")) {
		SC_HANDLE handleManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!handleManager) {
			Log(LogCritical, "cli") << "Failed to open service manager. Error code: " << GetLastError();
			return EXIT_FAILURE;
		}

		std::string service = vm["restart-service"].as<std::string>();

		SC_HANDLE handleService = OpenService(handleManager, service.c_str(), SERVICE_START | SERVICE_STOP);
		if (!handleService) {
			Log(LogCritical, "cli") << "Failed to open service handle of '" << service << "'. Error code: " << GetLastError();
			return EXIT_FAILURE;
		}

		SERVICE_STATUS serviceStatus;
		if (!ControlService(handleService, SERVICE_CONTROL_STOP, &serviceStatus)) {
			DWORD error = GetLastError();
			if (error = ERROR_SERVICE_NOT_ACTIVE)
				Log(LogInformation, "cli") << "Service '" << service << "' is not running.";
			else {
				Log(LogCritical, "cli") << "Failed to stop service. Error code: " << GetLastError();
				return EXIT_FAILURE;
			}
		}

		if (!StartService(handleService, 0, NULL)) {
			Log(LogCritical, "cli") << "Failed to start up service '" << service << "'. Error code: " << GetLastError();
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}
#endif /* _WIN32 */

	if (vm.count("validate")) {
		Log(LogInformation, "cli", "Finished validating the configuration file(s).");
		return EXIT_SUCCESS;
	}

#ifndef _WIN32
	if (vm.count("reload-internal")) {
		/* We went through validation and now ask the old process kindly to die */
		Log(LogInformation, "cli", "Requesting to take over.");
		int rc = kill(vm["reload-internal"].as<int>(), SIGUSR2);
		if (rc) {
			Log(LogCritical, "cli")
				<< "Failed to send signal to \"" << vm["reload-internal"].as<int>() <<  "\" with " << strerror(errno);
			return EXIT_FAILURE;
		}

		double start = Utility::GetTime();
		while (kill(vm["reload-internal"].as<int>(), SIGCHLD) == 0)
			Utility::Sleep(0.2);

		Log(LogNotice, "cli")
			<< "Waited for " << Utility::FormatDuration(Utility::GetTime() - start) << " on old process to exit.";
	}
#endif /* _WIN32 */

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
	sigaction(SIGHUP, &sa, nullptr);
#endif /* _WIN32 */

	ApiListener::UpdateObjectAuthority();

	return Application::GetInstance()->Run();
}
