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

#include "config/configcompilercontext.h"
#include "config/configcompiler.h"
#include "config/configitembuilder.h"
#include "base/application.h"
#include "base/logger.h"
#include "base/timer.h"
#include "base/utility.h"
#include "base/exception.h"
#include "base/convert.h"
#include "base/scriptvariable.h"
#include "base/context.h"
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

static po::variables_map g_AppParams;

#ifdef _WIN32
SERVICE_STATUS l_SvcStatus;
SERVICE_STATUS_HANDLE l_SvcStatusHandle;
#endif /* _WIN32 */

static String LoadAppType(const String& typeSpec)
{
	int index;

	Log(LogInformation, "icinga-app", "Loading application type: " + typeSpec);

	index = typeSpec.FindFirstOf('/');

	if (index == String::NPos)
		return typeSpec;

	String library = typeSpec.SubStr(0, index);

	(void) Utility::LoadExtensionLibrary(library);

	return typeSpec.SubStr(index + 1);
}

static bool LoadConfigFiles(const String& appType, ValidationType validate)
{
	ConfigCompilerContext::GetInstance()->Reset();

	if (g_AppParams.count("config") > 0) {
		BOOST_FOREACH(const String& configPath, g_AppParams["config"].as<std::vector<std::string> >()) {
			ConfigCompiler::CompileFile(configPath);
		}
	}

	String name, fragment;
	BOOST_FOREACH(boost::tie(name, fragment), ConfigFragmentRegistry::GetInstance()->GetItems()) {
		ConfigCompiler::CompileText(name, fragment);
	}

	ConfigItemBuilder::Ptr builder = make_shared<ConfigItemBuilder>();
	builder->SetType(appType);
	builder->SetName("application");
	ConfigItem::Ptr item = builder->Compile();
	item->Register();

	bool result = ConfigItem::ActivateItems(validate);

	int warnings = 0, errors = 0;

	BOOST_FOREACH(const ConfigCompilerMessage& message, ConfigCompilerContext::GetInstance()->GetMessages()) {
		std::ostringstream locbuf;
		ShowCodeFragment(locbuf, message.Location, true);
		String location = locbuf.str();

		String logmsg;

		if (!location.IsEmpty())
			logmsg = "Location:\n" + location;

		logmsg += String("\nConfig ") + (message.Error ? "error" : "warning") + ": " + message.Text;

		if (message.Error) {
			Log(LogCritical, "config", logmsg);
			errors++;
		} else {
			Log(LogWarning, "config", logmsg);
			warnings++;
		}
	}

	if (warnings > 0 || errors > 0) {
		LogSeverity severity;

		if (errors == 0)
			severity = LogWarning;
		else
			severity = LogCritical;

		Log(severity, "config", Convert::ToString(errors) + " errors, " + Convert::ToString(warnings) + " warnings.");
	}

	if (!result)
		return false;

	return true;
}

#ifndef _WIN32
static void SigHupHandler(int)
{
	Application::RequestRestart();
}
#endif /* _WIN32 */

static bool Daemonize(const String& stderrFile)
{
#ifndef _WIN32
	pid_t pid = fork();
	if (pid == -1) {
		return false;
	}

	if (pid)
		exit(0);

	int fdnull = open("/dev/null", O_RDWR);
	if (fdnull > 0) {
		if (fdnull != 0)
			dup2(fdnull, 0);

		if (fdnull != 1)
			dup2(fdnull, 1);

		if (fdnull > 2)
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

int Main(void)
{
	int argc = Application::GetArgC();
	char **argv = Application::GetArgV();

	Application::SetStartTime(Utility::GetTime());

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
			Application::DeclareLocalStateDir(prefix + "\\var");
			Application::DeclarePkgDataDir(prefix + "\\share\\icinga2");

			builtinPaths = false;
		}

		RegCloseKey(hKey);
	}

	if (builtinPaths) {
		Log(LogWarning, "icinga-app", "Registry key could not be read. Falling back to built-in paths.");

#endif /* _WIN32 */
		Application::DeclarePrefixDir(ICINGA_PREFIX);
		Application::DeclareSysconfDir(ICINGA_SYSCONFDIR);
		Application::DeclareLocalStateDir(ICINGA_LOCALSTATEDIR);
		Application::DeclarePkgDataDir(ICINGA_PKGDATADIR);
#ifdef _WIN32
	}
#endif /* _WIN32 */

	Application::DeclareApplicationType("icinga/IcingaApplication");

	po::options_description desc("Supported options");
	desc.add_options()
		("help", "show this help message")
		("version,V", "show version information")
		("library,l", po::value<std::vector<std::string> >(), "load a library")
		("include,I", po::value<std::vector<std::string> >(), "add include search directory")
		("define,D", po::value<std::vector<std::string> >(), "define a constant")
		("config,c", po::value<std::vector<std::string> >(), "parse a configuration file")
		("no-config,z", "start without a configuration file")
		("validate,C", "exit after validating the configuration")
		("no-validate,Z", "skip validating the configuration")
		("debug,x", "enable debugging")
		("errorlog,e", po::value<std::string>(), "log fatal errors to the specified log file (only works in combination with --daemonize)")
#ifndef _WIN32
		("daemonize,d", "detach from the controlling terminal")
		("user,u", po::value<std::string>(), "user to run Icinga as")
		("group,g", po::value<std::string>(), "group to run Icinga as")
#else /* _WIN32 */
		("scm", "run as a Windows service (must be the first argument if specified)")
		("scm-install", "installs Icinga 2 as a Windows service (must be the first argument if specified")
		("scm-uninstall", "uninstalls the Icinga 2 Windows service (must be the first argument if specified")
#endif /* _WIN32 */
	;

	try {
		po::store(po::parse_command_line(argc, argv, desc), g_AppParams);
	} catch (const std::exception& ex) {
		std::ostringstream msgbuf;
		msgbuf << "Error while parsing command-line options: " << ex.what();
		Log(LogCritical, "icinga-app", msgbuf.str());
		return EXIT_FAILURE;
	}

	po::notify(g_AppParams);

	if (g_AppParams.count("define")) {
		BOOST_FOREACH(const String& define, g_AppParams["define"].as<std::vector<std::string> >()) {
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

	Application::DeclareStatePath(Application::GetLocalStateDir() + "/lib/icinga2/icinga2.state");
	Application::DeclarePidPath(Application::GetLocalStateDir() + "/run/icinga2/icinga2.pid");

#ifndef _WIN32
	if (g_AppParams.count("group")) {
		String group = g_AppParams["group"].as<std::string>();

		errno = 0;
		struct group *gr = getgrnam(group.CStr());

		if (!gr) {
			if (errno == 0) {
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid group specified: " + group));
			} else {
				BOOST_THROW_EXCEPTION(posix_error()
					<< boost::errinfo_api_function("getgrnam")
					<< boost::errinfo_errno(errno));
			}
		}

		if (setgid(gr->gr_gid) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("setgid")
				<< boost::errinfo_errno(errno));
		}
	}

	if (g_AppParams.count("user")) {
		String user = g_AppParams["user"].as<std::string>();

		errno = 0;
		struct passwd *pw = getpwnam(user.CStr());

		if (!pw) {
			if (errno == 0) {
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid user specified: " + user));
			} else {
				BOOST_THROW_EXCEPTION(posix_error()
					<< boost::errinfo_api_function("getpwnam")
					<< boost::errinfo_errno(errno));
			}
		}

		if (setuid(pw->pw_uid) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("setuid")
				<< boost::errinfo_errno(errno));
		}
	}
#endif /* _WIN32 */

	if (g_AppParams.count("debug"))
		Application::SetDebugging(true);

	if (g_AppParams.count("help") || g_AppParams.count("version")) {
		String appName = Utility::BaseName(argv[0]);

		if (appName.GetLength() > 3 && appName.SubStr(0, 3) == "lt-")
			appName = appName.SubStr(3, appName.GetLength() - 3);

		std::cout << appName << " " << "- The Icinga 2 network monitoring daemon.";

		if (g_AppParams.count("version")) {
			std::cout << " (Version: " << Application::GetVersion() << ")";
			std::cout << std::endl
				<< "Copyright (c) 2012-2014 Icinga Development Team (http://www.icinga.org)" << std::endl
				<< "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl2.html>" << std::endl
				<< "This is free software: you are free to change and redistribute it." << std::endl
				<< "There is NO WARRANTY, to the extent permitted by law.";
		}

		std::cout << std::endl;

		if (g_AppParams.count("version"))
			return EXIT_SUCCESS;
	}

	if (g_AppParams.count("help")) {
		std::cout << std::endl
			<< desc << std::endl
			<< "Report bugs at <https://dev.icinga.org/>" << std::endl
			<< "Icinga home page: <http://www.icinga.org/>" << std::endl;
		return EXIT_SUCCESS;
	}

	ScriptVariable::Set("UseVfork", true, false, true);

	Application::MakeVariablesConstant();

	Log(LogInformation, "icinga-app", "Icinga application loader (version: " + Application::GetVersion() + ")");

	String appType = LoadAppType(Application::GetApplicationType());

	if (g_AppParams.count("library")) {
		BOOST_FOREACH(const String& libraryName, g_AppParams["library"].as<std::vector<std::string> >()) {
			(void)Utility::LoadExtensionLibrary(libraryName);
		}
	}

	ConfigCompiler::AddIncludeSearchDir(Application::GetPkgDataDir());

	if (g_AppParams.count("include")) {
		BOOST_FOREACH(const String& includePath, g_AppParams["include"].as<std::vector<std::string> >()) {
			ConfigCompiler::AddIncludeSearchDir(includePath);
		}
	}

	if (g_AppParams.count("no-config") == 0 && g_AppParams.count("config") == 0) {
		Log(LogCritical, "icinga-app", "You need to specify at least one config file (using the --config option).");

		return EXIT_FAILURE;
	}

	if (g_AppParams.count("daemonize")) {
		String errorLog;

		if (g_AppParams.count("errorlog"))
			errorLog = g_AppParams["errorlog"].as<std::string>();

		Daemonize(errorLog);
		Logger::DisableConsoleLog();
	}

	ValidationType validate = ValidateStart;

	if (g_AppParams.count("validate"))
		validate = ValidateOnly;

	if (g_AppParams.count("no-validate"))
		validate = ValidateNone;

	if (!LoadConfigFiles(appType, validate))
		return EXIT_FAILURE;

	if (validate == ValidateOnly) {
		Log(LogInformation, "icinga-app", "Finished validating the configuration file(s).");
		return EXIT_SUCCESS;
	}

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &SigHupHandler;
	sigaction(SIGHUP, &sa, NULL);
#endif /* _WIN32 */

	int rc = Application::GetInstance()->Run();

#ifndef _DEBUG
	_exit(rc); // Yay, our static destructors are pretty much beyond repair at this point.
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

	SC_HANDLE schService = OpenService(schSCManager, "icinga2", DELETE);

	if (schService != NULL) {
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

		SERVICE_DESCRIPTION sdDescription = { "The Icinga 2 monitoring application" };
		ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sdDescription);

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
		_exit(1);
	}
#endif /* _WIN32 */

	int rc = Main();
	exit(rc);
}
