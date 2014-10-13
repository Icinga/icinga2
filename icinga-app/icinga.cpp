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

#include "config/configcompilercontext.hpp"
#include "config/configcompiler.hpp"
#include "config/configitembuilder.hpp"
#include "base/application.hpp"
#include "base/logger.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/scriptvariable.hpp"
#include "base/context.hpp"
#include "base/clicommand.hpp"
#include "config.h"
#include <boost/program_options.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

#ifndef _WIN32
#	include <sys/types.h>
#	include <pwd.h>
#	include <grp.h>
#endif /* _WIN32 */

using namespace icinga;
namespace po = boost::program_options;

#ifdef _WIN32
SERVICE_STATUS l_SvcStatus;
SERVICE_STATUS_HANDLE l_SvcStatusHandle;
#endif /* _WIN32 */

int Main(void)
{
	int argc = Application::GetArgC();
	char **argv = Application::GetArgV();

	bool autocomplete = false;
	int autoindex = 0;

	if (argc >= 4 && strcmp(argv[1], "--autocomplete") == 0) {
		autocomplete = true;
		autoindex = Convert::ToLong(argv[2]);
		argc -= 3;
		argv += 3;
	}

	Application::SetStartTime(Utility::GetTime());

	if (!autocomplete)
		Application::SetResourceLimits();

	/* Set thread title. */
	Utility::SetThreadName("Main Thread", false);

	/* Install exception handlers to make debugging easier. */
	Application::InstallExceptionHandlers();

#ifdef _WIN32
	bool builtinPaths = true;

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Icinga Development Team\\ICINGA2", 0,
	    KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
		BYTE pvData[MAX_PATH];
		DWORD cbData = sizeof(pvData)-1;
		DWORD lType;
		if (RegQueryValueEx(hKey, NULL, NULL, &lType, pvData, &cbData) == ERROR_SUCCESS && lType == REG_SZ) {
			pvData[cbData] = '\0';

			String prefix = (char *)pvData;
			Application::DeclarePrefixDir(prefix);
			Application::DeclareSysconfDir(prefix + "\\etc");
			Application::DeclareRunDir(prefix + "\\var\\run");
			Application::DeclareLocalStateDir(prefix + "\\var");
			Application::DeclarePkgDataDir(prefix + "\\share\\icinga2");
			Application::DeclareIncludeConfDir(prefix + "\\share\\icinga2\\include");

			builtinPaths = false;
		}

		RegCloseKey(hKey);
	}

	if (builtinPaths) {
		Log(LogWarning, "icinga-app", "Registry key could not be read. Falling back to built-in paths.");

#endif /* _WIN32 */
		Application::DeclarePrefixDir(ICINGA_PREFIX);
		Application::DeclareSysconfDir(ICINGA_SYSCONFDIR);
		Application::DeclareRunDir(ICINGA_RUNDIR);
		Application::DeclareLocalStateDir(ICINGA_LOCALSTATEDIR);
		Application::DeclarePkgDataDir(ICINGA_PKGDATADIR);
		Application::DeclareIncludeConfDir(ICINGA_INCLUDECONFDIR);
#ifdef _WIN32
	}
#endif /* _WIN32 */

	Application::DeclareZonesDir(Application::GetSysconfDir() + "/icinga2/zones.d");
	Application::DeclareApplicationType("icinga/IcingaApplication");
	Application::DeclareRunAsUser(ICINGA_USER);
	Application::DeclareRunAsGroup(ICINGA_GROUP);

	LogSeverity logLevel = Logger::GetConsoleLogSeverity();
	Logger::SetConsoleLogSeverity(LogWarning);
	
	Utility::LoadExtensionLibrary("cli");

	po::options_description visibleDesc("Global options");

	visibleDesc.add_options()
		("help", "show this help message")
		("version,V", "show version information")
		("define,D", po::value<std::vector<std::string> >(), "define a constant")
		("library,l", po::value<std::vector<std::string> >(), "load a library")
		("include,I", po::value<std::vector<std::string> >(), "add include search directory")
		("log-level,x", po::value<std::string>(), "specify the log level for the console log")
#ifndef _WIN32
		("user,u", po::value<std::string>(), "user to run Icinga as")
		("group,g", po::value<std::string>(), "group to run Icinga as")
#endif /* _WIN32 */
	;

	po::options_description hiddenDesc("Hidden options");

	hiddenDesc.add_options()
		("no-stack-rlimit", "used internally, do not specify manually");
	
	String cmdname;
	CLICommand::Ptr command;
	po::variables_map vm;

	try {
		CLICommand::ParseCommand(argc, argv, visibleDesc, hiddenDesc, vm, cmdname, command, autocomplete);
	} catch (const std::exception& ex) {
		std::ostringstream msgbuf;
		msgbuf << "Error while parsing command-line options: " << ex.what();
		Log(LogCritical, "cli_daemon", msgbuf.str());
		return EXIT_FAILURE;
	}

	String initconfig = Application::GetSysconfDir() + "/icinga2/init.conf";

	if (Utility::PathExists(initconfig)) {
		ConfigCompilerContext::GetInstance()->Reset();
		ConfigCompiler::CompileFile(initconfig);
	}
	
	if (vm.count("define")) {
		BOOST_FOREACH(const String& define, vm["define"].as<std::vector<std::string> >()) {
			String key, value;
			size_t pos = define.FindFirstOf('=');
			if (pos != String::NPos) {
				key = define.SubStr(0, pos);
				value = define.SubStr(pos + 1);
			} else {
				key = define;
				value = "1";
			}
			ScriptVariable::Set(key, value);
		}
	}

	if (vm.count("group"))
		ScriptVariable::Set("RunAsGroup", String(vm["group"].as<std::string>()));

	if (vm.count("user"))
		ScriptVariable::Set("RunAsUser", String(vm["user"].as<std::string>()));

#ifndef _WIN32
	String group = Application::GetRunAsGroup();

	errno = 0;
	struct group *gr = getgrnam(group.CStr());

	if (!gr) {
		if (errno == 0) {
			std::ostringstream msgbuf;
			msgbuf << "Invalid group specified: " + group;
			Log(LogCritical, "cli",  msgbuf.str());
			return EXIT_FAILURE;
		} else {
			std::ostringstream msgbuf;
			msgbuf << "getgrnam() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			Log(LogCritical, "cli",  msgbuf.str());
			return EXIT_FAILURE;
		}
	}

	if (getgid() != gr->gr_gid) {
		if (!vm.count("reload-internal") && setgroups(0, NULL) < 0) {
			std::ostringstream msgbuf;
			msgbuf << "setgroups() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			Log(LogCritical, "cli",  msgbuf.str());
			return EXIT_FAILURE;
		}
	
		if (setgid(gr->gr_gid) < 0) {
			std::ostringstream msgbuf;
			msgbuf << "setgid() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			Log(LogCritical, "cli",  msgbuf.str());
			return EXIT_FAILURE;
		}
	}

	String user = Application::GetRunAsUser();

	errno = 0;
	struct passwd *pw = getpwnam(user.CStr());

	if (!pw) {
		if (errno == 0) {
			std::ostringstream msgbuf;
			msgbuf << "Invalid user specified: " + user;
			Log(LogCritical, "cli",  msgbuf.str());
			return EXIT_FAILURE;
		} else {
			std::ostringstream msgbuf;
			msgbuf << "getpwnam() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			Log(LogCritical, "cli",  msgbuf.str());
			return EXIT_FAILURE;
		}
	}

	// also activate the additional groups the configured user is member of
	if (getuid() != pw->pw_uid) {
		if (!vm.count("reload-internal") && initgroups(user.CStr(), pw->pw_gid) < 0) {
			std::ostringstream msgbuf;
			msgbuf << "initgroups() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			Log(LogCritical, "cli",  msgbuf.str());
			return EXIT_FAILURE;
		}
	
		if (setuid(pw->pw_uid) < 0) {
			std::ostringstream msgbuf;
			msgbuf << "setuid() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			Log(LogCritical, "cli",  msgbuf.str());
			return EXIT_FAILURE;
		}
	}
#endif /* _WIN32 */

	Application::DeclareStatePath(Application::GetLocalStateDir() + "/lib/icinga2/icinga2.state");
	Application::DeclareObjectsPath(Application::GetLocalStateDir() + "/cache/icinga2/icinga2.debug");
	Application::DeclarePidPath(Application::GetRunDir() + "/icinga2/icinga2.pid");

	ConfigCompiler::AddIncludeSearchDir(Application::GetIncludeConfDir());

	if (!autocomplete && vm.count("include")) {
		BOOST_FOREACH(const String& includePath, vm["include"].as<std::vector<std::string> >()) {
			ConfigCompiler::AddIncludeSearchDir(includePath);
		}
	}
	
	Logger::SetConsoleLogSeverity(logLevel);

	if (!autocomplete) {
		if (vm.count("log-level")) {
			String severity = vm["log-level"].as<std::string>();
	
			LogSeverity logLevel = LogInformation;
			try {
				logLevel = Logger::StringToSeverity(severity);
			} catch (std::exception&) {
				/* use the default */
				Log(LogWarning, "icinga", "Invalid log level set. Using default 'information'.");
			}
	
			Logger::SetConsoleLogSeverity(logLevel);
		}
	
		if (vm.count("library")) {
			BOOST_FOREACH(const String& libraryName, vm["library"].as<std::vector<std::string> >()) {
				(void)Utility::LoadExtensionLibrary(libraryName);
			}
		}
	
		if (!command || vm.count("help") || vm.count("version")) {
			String appName = Utility::BaseName(Application::GetArgV()[0]);
	
			if (appName.GetLength() > 3 && appName.SubStr(0, 3) == "lt-")
				appName = appName.SubStr(3, appName.GetLength() - 3);
	
			std::cout << appName << " " << "- The Icinga 2 network monitoring daemon.";
	
			if (!command || vm.count("help")) {
				std::cout << std::endl << std::endl
				    << "Usage:" << std::endl
				    << "  " << argv[0] << " ";
				    
				if (cmdname.IsEmpty())
					std::cout << "<command>";
				else
					std::cout << cmdname;
					
				std::cout << " [<arguments>]";
				
				if (command) {
					std::cout << std::endl << std::endl
						  << command->GetDescription();
				}
			}
			
			if (vm.count("version")) {
				std::cout << " (Version: " << Application::GetVersion() << ")";
				std::cout << std::endl
					<< "Copyright (c) 2012-2014 Icinga Development Team (http://www.icinga.org)" << std::endl
					<< "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl2.html>" << std::endl
					<< "This is free software: you are free to change and redistribute it." << std::endl
					<< "There is NO WARRANTY, to the extent permitted by law.";
			}
	
			std::cout << std::endl;
	
			if (vm.count("version")) {
				std::cout << std::endl;
	
				Application::DisplayInfoMessage(true);
	
				return EXIT_SUCCESS;
			}
		}
	
		if (!command || vm.count("help")) {
			if (!command) {
				std::cout << std::endl;
				CLICommand::ShowCommands(argc, argv, NULL);
			}
	
			std::cout << std::endl
				<< visibleDesc << std::endl
				<< "Report bugs at <https://dev.icinga.org/>" << std::endl
				<< "Icinga home page: <http://www.icinga.org/>" << std::endl;
			return EXIT_SUCCESS;
		}
	}

	int rc = 1;

	if (autocomplete) {
		CLICommand::ShowCommands(argc, argv, &visibleDesc, &hiddenDesc, true, autoindex);
		rc = 0;
	} else if (command)
		rc = command->Run(vm);

#ifndef _DEBUG
	Application::Exit(rc);
#endif /* _DEBUG */

	return rc;
}

#ifdef _WIN32
static int SetupService(bool install, int argc, char **argv)
{
	SC_HANDLE schSCManager = OpenSCManager(
		NULL,
		NULL,
		SC_MANAGER_ALL_ACCESS);

	if (NULL == schSCManager) {
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return 1;
	}

	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
		printf("Cannot install service (%d)\n", GetLastError());
		return 1;
	}

	String szArgs;
	szArgs = Utility::EscapeShellArg(szPath) + " --scm";

	for (int i = 0; i < argc; i++)
		szArgs += " " + Utility::EscapeShellArg(argv[i]);

	SC_HANDLE schService = OpenService(schSCManager, "icinga2", DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);

	if (schService != NULL) {
		SERVICE_STATUS status;
		ControlService(schService, SERVICE_CONTROL_STOP, &status);

		double start = Utility::GetTime();
		while (status.dwCurrentState != SERVICE_STOPPED) {
			double end = Utility::GetTime();

			if (end - start > 30) {
				printf("Could not stop the service.\n");
				break;
			}

			Utility::Sleep(5);

			if (!QueryServiceStatus(schService, &status)) {
				printf("QueryServiceStatus failed (%d)\n", GetLastError());
				return 1;
			}
		}

		if (!DeleteService(schService)) {
			printf("DeleteService failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return 1;
		}

		if (!install)
			printf("Service uninstalled successfully\n");

		CloseServiceHandle(schService);
	}

	if (install) {
		schService = CreateService(
			schSCManager,
			"icinga2",
			"Icinga 2",
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			szArgs.CStr(),
			NULL,
			NULL,
			NULL,
			"NT AUTHORITY\\NetworkService",
			NULL);

		if (schService == NULL) {
			printf("CreateService failed (%d)\n", GetLastError());
			CloseServiceHandle(schSCManager);
			return 1;
		} else
			printf("Service installed successfully\n");

		ChangeServiceConfig(schService, SERVICE_NO_CHANGE, SERVICE_AUTO_START,
		    SERVICE_ERROR_NORMAL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		SERVICE_DESCRIPTION sdDescription = { "The Icinga 2 monitoring application" };
		ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sdDescription);

		if (!StartService(schService, 0, NULL)) {
			printf("StartService failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return 1;
		}

		CloseServiceHandle(schService);
	}

	CloseServiceHandle(schSCManager);

	return 0;
}

VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	l_SvcStatus.dwCurrentState = dwCurrentState;
	l_SvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	l_SvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		l_SvcStatus.dwControlsAccepted = 0;
	else
		l_SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
	    (dwCurrentState == SERVICE_STOPPED))
		l_SvcStatus.dwCheckPoint = 0;
	else
		l_SvcStatus.dwCheckPoint = dwCheckPoint++;

	SetServiceStatus(l_SvcStatusHandle, &l_SvcStatus);
}

VOID WINAPI ServiceControlHandler(DWORD dwCtrl)
{
	if (dwCtrl == SERVICE_CONTROL_STOP) {
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		Application::RequestShutdown();
	}
}

VOID WINAPI ServiceMain(DWORD argc, LPSTR *argv)
{
	l_SvcStatusHandle = RegisterServiceCtrlHandler(
		"icinga2",
		ServiceControlHandler);

	l_SvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	l_SvcStatus.dwServiceSpecificExitCode = 0;

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	int rc = Main();

	ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, rc);
}
#endif /* _WIN32 */

/**
* Entry point for the Icinga application.
*
* @params argc Number of command line arguments.
* @params argv Command line arguments.
* @returns The application's exit status.
*/
int main(int argc, char **argv)
{
	/* must be called before using any other libbase functions */
	Application::InitializeBase();

	/* Set command-line arguments. */
	Application::SetArgC(argc);
	Application::SetArgV(argv);

#ifdef _WIN32
	if (argc > 1 && strcmp(argv[1], "--scm-install") == 0) {
		return SetupService(true, argc - 2, &argv[2]);
	}

	if (argc > 1 && strcmp(argv[1], "--scm-uninstall") == 0) {
		return SetupService(false, argc - 2, &argv[2]);
	}

	if (argc > 1 && strcmp(argv[1], "--scm") == 0) {
		SERVICE_TABLE_ENTRY dispatchTable[] = {
			{ "icinga2", ServiceMain },
			{ NULL, NULL }
		};

		StartServiceCtrlDispatcher(dispatchTable);
		Application::Exit(1);
	}
#endif /* _WIN32 */

	int rc = Main();

	Application::Exit(rc);
}
