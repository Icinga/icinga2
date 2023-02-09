/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/clicommand.hpp"
#include "config/configcompilercontext.hpp"
#include "config/configcompiler.hpp"
#include "config/configitembuilder.hpp"
#include "config/expression.hpp"
#include "base/application.hpp"
#include "base/configuration.hpp"
#include "base/logger.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/loader.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/scriptglobal.hpp"
#include "base/context.hpp"
#include "base/console.hpp"
#include "base/process.hpp"
#include "config.h"
#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
#include <thread>

#ifndef _WIN32
#	include <sys/types.h>
#	include <pwd.h>
#	include <grp.h>
#else
#	include <windows.h>
#	include <Lmcons.h>
#	include <Shellapi.h>
#	include <tchar.h>
#endif /* _WIN32 */

using namespace icinga;
namespace po = boost::program_options;

#ifdef _WIN32
static SERVICE_STATUS l_SvcStatus;
static SERVICE_STATUS_HANDLE l_SvcStatusHandle;
static HANDLE l_Job;
#endif /* _WIN32 */

static std::vector<String> GetLogLevelCompletionSuggestions(const String& arg)
{
	std::vector<String> result;

	String debugLevel = "debug";
	if (debugLevel.Find(arg) == 0)
		result.push_back(debugLevel);

	String noticeLevel = "notice";
	if (noticeLevel.Find(arg) == 0)
		result.push_back(noticeLevel);

	String informationLevel = "information";
	if (informationLevel.Find(arg) == 0)
		result.push_back(informationLevel);

	String warningLevel = "warning";
	if (warningLevel.Find(arg) == 0)
		result.push_back(warningLevel);

	String criticalLevel = "critical";
	if (criticalLevel.Find(arg) == 0)
		result.push_back(criticalLevel);

	return result;
}

static std::vector<String> GlobalArgumentCompletion(const String& argument, const String& word)
{
	if (argument == "include")
		return GetBashCompletionSuggestions("directory", word);
	else if (argument == "log-level")
		return GetLogLevelCompletionSuggestions(word);
	else
		return std::vector<String>();
}

static void HandleLegacyDefines()
{
#ifdef _WIN32
	String dataPrefix = Utility::GetIcingaDataPath();
#endif /* _WIN32 */

	Value localStateDir = Configuration::LocalStateDir;

	if (!localStateDir.IsEmpty()) {
		Log(LogWarning, "icinga-app")
			<< "Please do not set the deprecated 'LocalStateDir' constant,"
			<< " use the 'DataDir', 'LogDir', 'CacheDir' and 'SpoolDir' constants instead!"
			<< " For compatibility reasons, these are now set based on the 'LocalStateDir' constant.";

#ifdef _WIN32
		Configuration::DataDir = localStateDir + "\\lib\\icinga2";
		Configuration::LogDir = localStateDir + "\\log\\icinga2";
		Configuration::CacheDir = localStateDir + "\\cache\\icinga2";
		Configuration::SpoolDir = localStateDir + "\\spool\\icinga2";
	} else {
		Configuration::LocalStateDir = dataPrefix + "\\var";
#else /* _WIN32 */
		Configuration::DataDir = localStateDir + "/lib/icinga2";
		Configuration::LogDir = localStateDir + "/log/icinga2";
		Configuration::CacheDir = localStateDir + "/cache/icinga2";
		Configuration::SpoolDir = localStateDir + "/spool/icinga2";
	} else {
		Configuration::LocalStateDir = ICINGA_LOCALSTATEDIR;
#endif /* _WIN32 */
	}

	Value sysconfDir = Configuration::SysconfDir;
	if (!sysconfDir.IsEmpty()) {
		Log(LogWarning, "icinga-app")
			<< "Please do not set the deprecated 'Sysconfdir' constant, use the 'ConfigDir' constant instead! For compatibility reasons, their value is set based on the 'SysconfDir' constant.";

#ifdef _WIN32
		Configuration::ConfigDir = sysconfDir + "\\icinga2";
	} else {
		Configuration::SysconfDir = dataPrefix + "\\etc";
#else /* _WIN32 */
		Configuration::ConfigDir = sysconfDir + "/icinga2";
	} else {
		Configuration::SysconfDir = ICINGA_SYSCONFDIR;
#endif /* _WIN32 */
	}

	Value runDir = Configuration::RunDir;
	if (!runDir.IsEmpty()) {
		Log(LogWarning, "icinga-app")
			<< "Please do not set the deprecated 'RunDir' constant, use the 'InitRunDir' constant instead! For compatibility reasons, their value is set based on the 'RunDir' constant.";

#ifdef _WIN32
		Configuration::InitRunDir = runDir + "\\icinga2";
	} else {
		Configuration::RunDir = dataPrefix + "\\var\\run";
#else /* _WIN32 */
		Configuration::InitRunDir = runDir + "/icinga2";
	} else {
		Configuration::RunDir = ICINGA_RUNDIR;
#endif /* _WIN32 */
	}
}

static int Main()
{
	int argc = Application::GetArgC();
	char **argv = Application::GetArgV();

	bool autocomplete = false;
	int autoindex = 0;

	if (argc >= 4 && strcmp(argv[1], "--autocomplete") == 0) {
		autocomplete = true;

		try {
			autoindex = Convert::ToLong(argv[2]);
		} catch (const std::invalid_argument&) {
			Log(LogCritical, "icinga-app")
				<< "Invalid index for --autocomplete: " << argv[2];
			return EXIT_FAILURE;
		}

		argc -= 3;
		argv += 3;
	}

	/* Set thread title. */
	Utility::SetThreadName("Main Thread", false);

	/* Install exception handlers to make debugging easier. */
	Application::InstallExceptionHandlers();

#ifdef _WIN32
	bool builtinPaths = true;

	/* Programm install location, C:/Program Files/Icinga2 */
	String binaryPrefix = Utility::GetIcingaInstallPath();
	/* Returns the datapath for daemons, %PROGRAMDATA%/icinga2 */
	String dataPrefix = Utility::GetIcingaDataPath();

	if (!binaryPrefix.IsEmpty() && !dataPrefix.IsEmpty()) {
		Configuration::ProgramData = dataPrefix;

		Configuration::ConfigDir = dataPrefix + "\\etc\\icinga2";

		Configuration::DataDir =  dataPrefix + "\\var\\lib\\icinga2";
		Configuration::LogDir = dataPrefix + "\\var\\log\\icinga2";
		Configuration::CacheDir = dataPrefix + "\\var\\cache\\icinga2";
		Configuration::SpoolDir = dataPrefix + "\\var\\spool\\icinga2";

		Configuration::PrefixDir = binaryPrefix;

		/* Internal constants. */
		Configuration::PkgDataDir = binaryPrefix + "\\share\\icinga2";
		Configuration::IncludeConfDir = binaryPrefix + "\\share\\icinga2\\include";

		Configuration::InitRunDir = dataPrefix + "\\var\\run\\icinga2";
	} else {
		Log(LogWarning, "icinga-app", "Registry key could not be read. Falling back to built-in paths.");

#endif /* _WIN32 */
		Configuration::ConfigDir = ICINGA_CONFIGDIR;

		Configuration::DataDir = ICINGA_DATADIR;
		Configuration::LogDir = ICINGA_LOGDIR;
		Configuration::CacheDir = ICINGA_CACHEDIR;
		Configuration::SpoolDir = ICINGA_SPOOLDIR;

		Configuration::PrefixDir = ICINGA_PREFIX;

		/* Internal constants. */
		Configuration::PkgDataDir = ICINGA_PKGDATADIR;
		Configuration::IncludeConfDir = ICINGA_INCLUDECONFDIR;

		Configuration::InitRunDir = ICINGA_INITRUNDIR;

#ifdef _WIN32
	}
#endif /* _WIN32 */

	Configuration::ZonesDir = Configuration::ConfigDir + "/zones.d";

	String icingaUser = Utility::GetFromEnvironment("ICINGA2_USER");
	if (icingaUser.IsEmpty())
		icingaUser = ICINGA_USER;

	String icingaGroup = Utility::GetFromEnvironment("ICINGA2_GROUP");
	if (icingaGroup.IsEmpty())
		icingaGroup = ICINGA_GROUP;

	Configuration::RunAsUser = icingaUser;
	Configuration::RunAsGroup = icingaGroup;

	if (!autocomplete) {
#ifdef RLIMIT_NOFILE
		String rLimitFiles = Utility::GetFromEnvironment("ICINGA2_RLIMIT_FILES");
		if (rLimitFiles.IsEmpty())
			Configuration::RLimitFiles = Application::GetDefaultRLimitFiles();
		else {
			try {
				Configuration::RLimitFiles = Convert::ToLong(rLimitFiles);
			} catch (const std::invalid_argument& ex) {
				std::cout
					<< "Error setting \"ICINGA2_RLIMIT_FILES\": " << ex.what() << '\n';
				return EXIT_FAILURE;
			}
		}
#endif /* RLIMIT_NOFILE */

#ifdef RLIMIT_NPROC
		String rLimitProcesses = Utility::GetFromEnvironment("ICINGA2_RLIMIT_PROCESSES");
		if (rLimitProcesses.IsEmpty())
			Configuration::RLimitProcesses = Application::GetDefaultRLimitProcesses();
		else {
			try {
				Configuration::RLimitProcesses = Convert::ToLong(rLimitProcesses);
			} catch (const std::invalid_argument& ex) {
				std::cout
					<< "Error setting \"ICINGA2_RLIMIT_PROCESSES\": " << ex.what() << '\n';
				return EXIT_FAILURE;
			}
		}
#endif /* RLIMIT_NPROC */

#ifdef RLIMIT_STACK
		String rLimitStack = Utility::GetFromEnvironment("ICINGA2_RLIMIT_STACK");
		if (rLimitStack.IsEmpty())
			Configuration::RLimitStack = Application::GetDefaultRLimitStack();
		else {
			try {
				Configuration::RLimitStack = Convert::ToLong(rLimitStack);
			} catch (const std::invalid_argument& ex) {
				std::cout
					<< "Error setting \"ICINGA2_RLIMIT_STACK\": " << ex.what() << '\n';
				return EXIT_FAILURE;
			}
		}
#endif /* RLIMIT_STACK */
	}

	/* Calculate additional global constants. */
	ScriptGlobal::Set("System.PlatformKernel", Utility::GetPlatformKernel(), true);
	ScriptGlobal::Set("System.PlatformKernelVersion", Utility::GetPlatformKernelVersion(), true);
	ScriptGlobal::Set("System.PlatformName", Utility::GetPlatformName(), true);
	ScriptGlobal::Set("System.PlatformVersion", Utility::GetPlatformVersion(), true);
	ScriptGlobal::Set("System.PlatformArchitecture", Utility::GetPlatformArchitecture(), true);

	ScriptGlobal::Set("System.BuildHostName", ICINGA_BUILD_HOST_NAME, true);
	ScriptGlobal::Set("System.BuildCompilerName", ICINGA_BUILD_COMPILER_NAME, true);
	ScriptGlobal::Set("System.BuildCompilerVersion", ICINGA_BUILD_COMPILER_VERSION, true);

	if (!autocomplete)
		Application::SetResourceLimits();

	LogSeverity logLevel = Logger::GetConsoleLogSeverity();
	Logger::SetConsoleLogSeverity(LogWarning);

	po::options_description visibleDesc("Global options");

	visibleDesc.add_options()
		("help,h", "show this help message")
		("version,V", "show version information")
#ifndef _WIN32
		("color", "use VT100 color codes even when stdout is not a terminal")
#endif /* _WIN32 */
		("define,D", po::value<std::vector<std::string> >(), "define a constant")
		("include,I", po::value<std::vector<std::string> >(), "add include search directory")
		("log-level,x", po::value<std::string>(), "specify the log level for the console log.\n"
			"The valid value is either debug, notice, information (default), warning, or critical")
		("script-debugger,X", "whether to enable the script debugger");

	po::options_description hiddenDesc("Hidden options");

	hiddenDesc.add_options()
		("no-stack-rlimit", "used internally, do not specify manually")
		("arg", po::value<std::vector<std::string> >(), "positional argument");

	po::positional_options_description positionalDesc;
	positionalDesc.add("arg", -1);

	String cmdname;
	CLICommand::Ptr command;
	po::variables_map vm;

	try {
		CLICommand::ParseCommand(argc, argv, visibleDesc, hiddenDesc, positionalDesc,
			vm, cmdname, command, autocomplete);
	} catch (const std::exception& ex) {
		Log(LogCritical, "icinga-app")
			<< "Error while parsing command-line options: " << ex.what();
		return EXIT_FAILURE;
	}

#ifdef _WIN32
	char username[UNLEN + 1];
	DWORD usernameLen = UNLEN + 1;
	GetUserName(username, &usernameLen);

	std::ifstream userFile;

	/* The implicit string assignment is needed for Windows builds. */
	String configDir = Configuration::ConfigDir;
	userFile.open(configDir + "/user");

	if (userFile && command && !Application::IsProcessElevated()) {
		std::string userLine;
		if (std::getline(userFile, userLine)) {
			userFile.close();

			std::vector<std::string> strs;
			boost::split(strs, userLine, boost::is_any_of("\\"));

			if (username != strs[1] && command->GetImpersonationLevel() == ImpersonationLevel::ImpersonateIcinga
				|| command->GetImpersonationLevel() == ImpersonationLevel::ImpersonateRoot) {
				TCHAR szPath[MAX_PATH];

				if (GetModuleFileName(nullptr, szPath, ARRAYSIZE(szPath))) {
					SHELLEXECUTEINFO sei = { sizeof(sei) };
					sei.lpVerb = _T("runas");
					sei.lpFile = "cmd.exe";
					sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
					sei.nShow = SW_SHOW;

					std::stringstream parameters;

					parameters << "/C " << "\"" << szPath << "\"" << " ";

					for (int i = 1; i < argc; i++) {
						if (i != 1)
							parameters << " ";
						parameters << argv[i];
					}

					parameters << " & SET exitcode=%errorlevel%";
					parameters << " & pause";
					parameters << " & EXIT /B %exitcode%";

					std::string str = parameters.str();
					LPCSTR cstr = str.c_str();

					sei.lpParameters = cstr;

					if (!ShellExecuteEx(&sei)) {
						DWORD dwError = GetLastError();
						if (dwError == ERROR_CANCELLED)
							Application::Exit(0);
					} else {
						WaitForSingleObject(sei.hProcess, INFINITE);

						DWORD exitCode;
						GetExitCodeProcess(sei.hProcess, &exitCode);

						CloseHandle(sei.hProcess);

						Application::Exit(exitCode);
					}
				}
			}
		} else {
			userFile.close();
		}
	}
#endif /* _WIN32 */

#ifndef _WIN32
	if (vm.count("color")) {
		Console::SetType(std::cout, Console_VT100);
		Console::SetType(std::cerr, Console_VT100);
	}
#endif /* _WIN32 */

	if (vm.count("define")) {
		for (const String& define : vm["define"].as<std::vector<std::string> >()) {
			String key, value;
			size_t pos = define.FindFirstOf('=');
			if (pos != String::NPos) {
				key = define.SubStr(0, pos);
				value = define.SubStr(pos + 1);
			} else {
				key = define;
				value = "1";
			}

			std::vector<String> keyTokens = key.Split(".");

			std::unique_ptr<Expression> expr;
			std::unique_ptr<VariableExpression> varExpr{new VariableExpression(keyTokens[0], {}, DebugInfo())};
			expr = std::move(varExpr);

			for (size_t i = 1; i < keyTokens.size(); i++) {
				std::unique_ptr<IndexerExpression> indexerExpr{new IndexerExpression(std::move(expr), MakeLiteral(keyTokens[i]))};
				indexerExpr->SetOverrideFrozen();
				expr = std::move(indexerExpr);
			}

			std::unique_ptr<SetExpression> setExpr{new SetExpression(std::move(expr), OpSetLiteral, MakeLiteral(value))};
			setExpr->SetOverrideFrozen();

			ScriptFrame frame(true);
			setExpr->Evaluate(frame);
		}
	}

	Configuration::SetReadOnly(true);

	/* Ensure that all defined constants work in the way we expect them. */
	HandleLegacyDefines();

	if (vm.count("script-debugger"))
		Application::SetScriptDebuggerEnabled(true);

	Configuration::StatePath = Configuration::DataDir + "/icinga2.state";
	Configuration::ModAttrPath = Configuration::DataDir + "/modified-attributes.conf";
	Configuration::ObjectsPath = Configuration::CacheDir + "/icinga2.debug";
	Configuration::VarsPath = Configuration::CacheDir + "/icinga2.vars";
	Configuration::PidPath = Configuration::InitRunDir + "/icinga2.pid";

	ConfigCompiler::AddIncludeSearchDir(Configuration::IncludeConfDir);

	if (!autocomplete && vm.count("include")) {
		for (const String& includePath : vm["include"].as<std::vector<std::string> >()) {
			ConfigCompiler::AddIncludeSearchDir(includePath);
		}
	}

	if (!autocomplete) {
		Logger::SetConsoleLogSeverity(logLevel);

		if (vm.count("log-level")) {
			String severity = vm["log-level"].as<std::string>();

			LogSeverity logLevel = LogInformation;
			try {
				logLevel = Logger::StringToSeverity(severity);
			} catch (std::exception&) {
				/* Inform user and exit */
				Log(LogCritical, "icinga-app", "Invalid log level set. Default is 'information'.");
				return EXIT_FAILURE;
			}

			Logger::SetConsoleLogSeverity(logLevel);
		}

		if (!command || vm.count("help") || vm.count("version")) {
			String appName;

			try {
				appName = Utility::BaseName(Application::GetArgV()[0]);
			} catch (const std::bad_alloc&) {
				Log(LogCritical, "icinga-app", "Allocation failed.");
				return EXIT_FAILURE;
			}

			if (appName.GetLength() > 3 && appName.SubStr(0, 3) == "lt-")
				appName = appName.SubStr(3, appName.GetLength() - 3);

			std::cout << appName << " " << "- The Icinga 2 network monitoring daemon (version: "
				<< ConsoleColorTag(vm.count("version") ? Console_ForegroundRed : Console_Normal)
				<< Application::GetAppVersion()
#ifdef I2_DEBUG
				<< "; debug"
#endif /* I2_DEBUG */
				<< ConsoleColorTag(Console_Normal)
				<< ")" << std::endl << std::endl;

			if ((!command || vm.count("help")) && !vm.count("version")) {
				std::cout << "Usage:" << std::endl
					<< "  " << Utility::BaseName(argv[0]) << " ";

				if (cmdname.IsEmpty())
					std::cout << "<command>";
				else
					std::cout << cmdname;

				std::cout << " [<arguments>]" << std::endl;

				if (command) {
					std::cout << std::endl
						<< command->GetDescription() << std::endl;
				}
			}

			if (vm.count("version")) {
				std::cout << "Copyright (c) 2012-" << Utility::FormatDateTime("%Y", Utility::GetTime())
					<< " Icinga GmbH (https://icinga.com/)" << std::endl
					<< "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl2.html>" << std::endl
					<< "This is free software: you are free to change and redistribute it." << std::endl
					<< "There is NO WARRANTY, to the extent permitted by law.";
			}

			std::cout << std::endl;

			if (vm.count("version")) {
				std::cout << std::endl;

				Application::DisplayInfoMessage(std::cout, true);

				return EXIT_SUCCESS;
			}
		}

		if (!command || vm.count("help")) {
			if (!command)
				CLICommand::ShowCommands(argc, argv, nullptr);

			std::cout << visibleDesc << std::endl
				<< "Report bugs at <https://github.com/Icinga/icinga2>" << std::endl
				<< "Get support: <https://icinga.com/support/>" << std::endl
				<< "Documentation: <https://icinga.com/docs/>" << std::endl
				<< "Icinga home page: <https://icinga.com/>" << std::endl;
			return EXIT_SUCCESS;
		}
	}

	int rc = 1;

	if (autocomplete) {
		CLICommand::ShowCommands(argc, argv, &visibleDesc, &hiddenDesc,
			&GlobalArgumentCompletion, true, autoindex);
		rc = 0;
	} else if (command) {
		Logger::DisableTimestamp();
#ifndef _WIN32
		if (command->GetImpersonationLevel() == ImpersonateRoot) {
			if (getuid() != 0) {
				Log(LogCritical, "cli", "This command must be run as root.");
				return 0;
			}
		} else if (command && command->GetImpersonationLevel() == ImpersonateIcinga) {
			String group = Configuration::RunAsGroup;
			String user = Configuration::RunAsUser;

			errno = 0;
			struct group *gr = getgrnam(group.CStr());

			if (!gr) {
				if (errno == 0) {
					Log(LogCritical, "cli")
						<< "Invalid group specified: " << group;
					return EXIT_FAILURE;
				} else {
					Log(LogCritical, "cli")
						<< "getgrnam() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
					return EXIT_FAILURE;
				}
			}

			if (getgid() != gr->gr_gid) {
				if (!vm.count("reload-internal") && setgroups(0, nullptr) < 0) {
					Log(LogCritical, "cli")
						<< "setgroups() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
					Log(LogCritical, "cli")
						<< "Please re-run this command as a privileged user or using the \"" << user << "\" account.";
					return EXIT_FAILURE;
				}

				if (setgid(gr->gr_gid) < 0) {
					Log(LogCritical, "cli")
						<< "setgid() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
					return EXIT_FAILURE;
				}
			}

			errno = 0;
			struct passwd *pw = getpwnam(user.CStr());

			if (!pw) {
				if (errno == 0) {
					Log(LogCritical, "cli")
						<< "Invalid user specified: " << user;
					return EXIT_FAILURE;
				} else {
					Log(LogCritical, "cli")
						<< "getpwnam() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
					return EXIT_FAILURE;
				}
			}

			// also activate the additional groups the configured user is member of
			if (getuid() != pw->pw_uid) {
				if (!vm.count("reload-internal") && initgroups(user.CStr(), pw->pw_gid) < 0) {
					Log(LogCritical, "cli")
						<< "initgroups() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
					Log(LogCritical, "cli")
						<< "Please re-run this command as a privileged user or using the \"" << user << "\" account.";
					return EXIT_FAILURE;
				}

				if (setuid(pw->pw_uid) < 0) {
					Log(LogCritical, "cli")
						<< "setuid() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
					Log(LogCritical, "cli")
						<< "Please re-run this command as a privileged user or using the \"" << user << "\" account.";
					return EXIT_FAILURE;
				}
			}
		}
#endif /* _WIN32 */

		std::vector<std::string> args;
		if (vm.count("arg"))
			args = vm["arg"].as<std::vector<std::string> >();

		if (static_cast<int>(args.size()) < command->GetMinArguments()) {
			Log(LogCritical, "cli")
				<< "Too few arguments. Command needs at least " << command->GetMinArguments()
				<< " argument" << (command->GetMinArguments() != 1 ? "s" : "") << ".";
			return EXIT_FAILURE;
		}

		if (command->GetMaxArguments() >= 0 && static_cast<int>(args.size()) > command->GetMaxArguments()) {
			Log(LogCritical, "cli")
				<< "Too many arguments. At most " << command->GetMaxArguments()
				<< " argument" << (command->GetMaxArguments() != 1 ? "s" : "") << " may be specified.";
			return EXIT_FAILURE;
		}

		rc = command->Run(vm, args);
	}

	return rc;
}

#ifdef _WIN32
static int SetupService(bool install, int argc, char **argv)
{
	SC_HANDLE schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

	if (!schSCManager) {
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return 1;
	}

	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(nullptr, szPath, MAX_PATH)) {
		printf("Cannot install service (%d)\n", GetLastError());
		return 1;
	}

	String szArgs;
	szArgs = Utility::EscapeShellArg(szPath) + " --scm";

	std::string scmUser = "NT AUTHORITY\\NetworkService";
	std::ifstream initf(Utility::GetIcingaDataPath() + "\\etc\\icinga2\\user");
	if (initf.good()) {
		std::getline(initf, scmUser);
	}
	initf.close();

	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "--scm-user") && i + 1 < argc) {
			scmUser = argv[i + 1];
			i++;
		} else
			szArgs += " " + Utility::EscapeShellArg(argv[i]);
	}

	SC_HANDLE schService = OpenService(schSCManager, "icinga2", SERVICE_ALL_ACCESS);

	if (schService) {
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
	} else if (install) {
		schService = CreateService(
			schSCManager,
			"icinga2",
			"Icinga 2",
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			szArgs.CStr(),
			nullptr,
			nullptr,
			nullptr,
			scmUser.c_str(),
			nullptr);

		if (!schService) {
			printf("CreateService failed (%d)\n", GetLastError());
			CloseServiceHandle(schSCManager);
			return 1;
		}
	} else {
		printf("Service isn't installed.\n");
		CloseServiceHandle(schSCManager);
		return 0;
	}

	if (!install) {
		if (!DeleteService(schService)) {
			printf("DeleteService failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return 1;
		}

		printf("Service uninstalled successfully\n");
	} else {
		if (!ChangeServiceConfig(schService, SERVICE_NO_CHANGE, SERVICE_AUTO_START,
			SERVICE_ERROR_NORMAL, szArgs.CStr(), nullptr, nullptr, nullptr, scmUser.c_str(), nullptr, nullptr)) {
			printf("ChangeServiceConfig failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return 1;
		}

		SERVICE_DESCRIPTION sdDescription = { "The Icinga 2 monitoring application" };
		if(!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sdDescription)) {
			printf("ChangeServiceConfig2 failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return 1;
		}

		if (!StartService(schService, 0, nullptr)) {
			printf("StartService failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return 1;
		}

		std::cout << "Service successfully installed for user '" << scmUser << "'\n";

		String userFilePath = Utility::GetIcingaDataPath() + "\\etc\\icinga2\\user";

		std::ofstream fuser(userFilePath.CStr(), std::ios::out | std::ios::trunc);
		if (fuser)
			fuser << scmUser;
		else
			std::cout << "Could not write user to " << userFilePath << "\n";
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 0;
}

static VOID ReportSvcStatus(DWORD dwCurrentState,
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

static VOID WINAPI ServiceControlHandler(DWORD dwCtrl)
{
	if (dwCtrl == SERVICE_CONTROL_STOP) {
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		TerminateJobObject(l_Job, 0);
	}
}

static VOID WINAPI ServiceMain(DWORD argc, LPSTR *argv)
{
	l_SvcStatusHandle = RegisterServiceCtrlHandler(
		"icinga2",
		ServiceControlHandler);

	l_SvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	l_SvcStatus.dwServiceSpecificExitCode = 0;

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	l_Job = CreateJobObject(nullptr, nullptr);

	for (;;) {
		LPSTR arg = argv[0];
		String args;
		int uargc = Application::GetArgC();
		char **uargv = Application::GetArgV();

		args += Utility::EscapeShellArg(Application::GetExePath(uargv[0]));

		for (int i = 2; i < uargc && uargv[i]; i++) {
			if (args != "")
				args += " ";

			args += Utility::EscapeShellArg(uargv[i]);
		}

		STARTUPINFO si = { sizeof(si) };
		PROCESS_INFORMATION pi;

		char *uargs = strdup(args.CStr());

		BOOL res = CreateProcess(nullptr, uargs, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);

		free(uargs);

		if (!res)
			break;

		CloseHandle(pi.hThread);

		AssignProcessToJobObject(l_Job, pi.hProcess);

		if (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0)
			break;

		DWORD exitStatus;

		if (!GetExitCodeProcess(pi.hProcess, &exitStatus))
			break;

		if (exitStatus != 7)
			break;
	}

	TerminateJobObject(l_Job, 0);

	CloseHandle(l_Job);

	ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);

	Application::Exit(0);
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
#ifndef _WIN32
	String keepFDs = Utility::GetFromEnvironment("ICINGA2_KEEP_FDS");
	if (keepFDs.IsEmpty()) {
		rlimit rl;
		if (getrlimit(RLIMIT_NOFILE, &rl) >= 0) {
			rlim_t maxfds = rl.rlim_max;

			if (maxfds == RLIM_INFINITY)
				maxfds = 65536;

			for (rlim_t i = 3; i < maxfds; i++) {
				int rc = close(i);

#ifdef I2_DEBUG
				if (rc >= 0)
					std::cerr << "Closed FD " << i << " which we inherited from our parent process." << std::endl;
#else /* I2_DEBUG */
				(void)rc;
#endif /* I2_DEBUG */
			}
		}
	}
#endif /* _WIN32 */

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
			{ nullptr, nullptr }
		};

		StartServiceCtrlDispatcher(dispatchTable);
		Application::Exit(EXIT_FAILURE);
	}
#endif /* _WIN32 */

	int rc = Main();

	Application::Exit(rc);
}
