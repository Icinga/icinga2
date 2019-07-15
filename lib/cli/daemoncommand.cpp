/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/daemoncommand.hpp"
#include "cli/daemonutility.hpp"
#include "remote/apilistener.hpp"
#include "remote/configobjectutility.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "config/configitembuilder.hpp"
#include "base/defer.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/scriptglobal.hpp"
#include "base/context.hpp"
#include "config.h"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>

#ifndef _WIN32
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif /* _WIN32 */

using namespace icinga;
namespace po = boost::program_options;

static po::variables_map g_AppParams;

REGISTER_CLICOMMAND("daemon", DaemonCommand);

/*
 * Daemonize().  On error, this function logs by itself and exits (i.e. does not return).
 *
 * Implementation note: We're only supposed to call exit() in one of the forked processes.
 * The other process calls _exit().  This prevents issues with exit handlers like atexit().
 */
static void Daemonize() noexcept
{
#ifndef _WIN32
	try {
		Application::UninitializeBase();
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
			<< "Failed to stop thread pool before daemonizing, unexpected error: " << DiagnosticInformation(ex);
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid == -1) {
		Log(LogCritical, "cli")
			<< "fork() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
		exit(EXIT_FAILURE);
	}

	if (pid) {
		// systemd requires that the pidfile of the daemon is written before the forking
		// process terminates. So wait till either the forked daemon has written a pidfile or died.

		int status;
		int ret;
		pid_t readpid;
		do {
			Utility::Sleep(0.1);

			readpid = Application::ReadPidFile(Configuration::PidPath);
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

	Log(LogDebug, "Daemonize()")
		<< "Child process with PID " << Utility::GetPid() << " continues; re-initializing base.";

	// Detach from controlling terminal
	pid_t sid = setsid();
	if (sid == -1) {
		Log(LogCritical, "cli")
			<< "setsid() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
		exit(EXIT_FAILURE);
	}

	try {
		Application::InitializeBase();
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
			<< "Failed to re-initialize thread pool after daemonizing: " << DiagnosticInformation(ex);
		exit(EXIT_FAILURE);
	}
#endif /* _WIN32 */
}

static void CloseStdIO(const String& stderrFile)
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
#endif
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
		("errorlog,e", po::value<std::string>(), "log fatal errors to the specified log file (only works in combination with --daemonize or --close-stdio)")
#ifndef _WIN32
		("daemonize,d", "detach from the controlling terminal")
		("close-stdio", "do not log to stdout (or stderr) after startup")
#endif /* _WIN32 */
	;
}

std::vector<String> DaemonCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "config" || argument == "errorlog")
		return GetBashCompletionSuggestions("file", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

#ifndef _WIN32
pid_t l_UmbrellaPid = 0;
static std::atomic<bool> l_AllowedToWork (false);
#endif /* _WIN32 */

static inline
int RunWorker(const std::vector<std::string>& configs)
{
	Log(LogInformation, "cli", "Loading configuration file(s).");

	{
		std::vector<ConfigItem::Ptr> newItems;

		if (!DaemonUtility::LoadConfigFiles(configs, newItems, Configuration::ObjectsPath, Configuration::VarsPath))
			return EXIT_FAILURE;

#ifndef _WIN32
		// Notify umbrella process about the config loading success.
		(void)kill(l_UmbrellaPid, SIGUSR2);

		while (!l_AllowedToWork.load()) {
			Utility::Sleep(0.2);
		}
#endif /* _WIN32 */

		/* restore the previous program state */
		try {
			ConfigObject::RestoreObjects(Configuration::StatePath);
		} catch (const std::exception& ex) {
			Log(LogCritical, "cli")
				<< "Failed to restore state file: " << DiagnosticInformation(ex);
			return EXIT_FAILURE;
		}

		WorkQueue upq(25000, Configuration::Concurrency);
		upq.SetName("DaemonCommand::Run");

		// activate config only after daemonization: it starts threads and that is not compatible with fork()
		if (!ConfigItem::ActivateItems(upq, newItems, false, false, true)) {
			Log(LogCritical, "cli", "Error activating configuration.");
			return EXIT_FAILURE;
		}
	}

	/* Create the internal API object storage. Do this here too with setups without API. */
	ConfigObjectUtility::CreateStorage();

	/* Remove ignored Downtime/Comment objects. */
	try {
		String configDir = ConfigObjectUtility::GetConfigDir();
		ConfigItem::RemoveIgnoredItems(configDir);
	} catch (const std::exception& ex) {
		Log(LogNotice, "cli")
			<< "Cannot clean ignored downtimes/comments: " << ex.what();
	}

	ApiListener::UpdateObjectAuthority();

	return Application::GetInstance()->Run();
}

#ifndef _WIN32
enum class UnixWorkerState : uint_fast8_t
{
	Pending,
	LoadedConfig,
	Failed
};

static const sigset_t l_UnixWorkerSignals = ([]() -> sigset_t {
	sigset_t s;

	(void)sigemptyset(&s);
	(void)sigaddset(&s, SIGCHLD);
	(void)sigaddset(&s, SIGUSR2);
	(void)sigaddset(&s, SIGINT);
	(void)sigaddset(&s, SIGTERM);
	(void)sigaddset(&s, SIGHUP);

	return s;
})();

static std::atomic<pid_t> l_CurrentlyStartingUnixWorkerPid (-1);
static std::atomic<UnixWorkerState> l_CurrentlyStartingUnixWorkerState (UnixWorkerState::Pending);
static std::atomic<int> l_TermSignal (-1);
static std::atomic<bool> l_RequestedReload (false);

static void UmbrellaSignalHandler(int num, siginfo_t *info, void*)
{
	switch (num) {
		case SIGUSR2:
			if (l_CurrentlyStartingUnixWorkerState.load() == UnixWorkerState::Pending
				&& info->si_pid == l_CurrentlyStartingUnixWorkerPid.load()) {
				l_CurrentlyStartingUnixWorkerState.store(UnixWorkerState::LoadedConfig);
			}
			break;
		case SIGCHLD:
			if (l_CurrentlyStartingUnixWorkerState.load() == UnixWorkerState::Pending
				&& info->si_pid == l_CurrentlyStartingUnixWorkerPid.load()) {
				l_CurrentlyStartingUnixWorkerState.store(UnixWorkerState::Failed);
			}
			break;
		case SIGINT:
		case SIGTERM:
			{
				struct sigaction sa;
				memset(&sa, 0, sizeof(sa));

				sa.sa_handler = SIG_DFL;

				(void)sigaction(num, &sa, nullptr);
			}

			l_TermSignal.store(num);
			break;
		case SIGHUP:
			l_RequestedReload.store(true);
			break;
		default:
			VERIFY(!"Caught unexpected signal");
	}
}

static void WorkerSignalHandler(int num, siginfo_t *info, void*)
{
	switch (num) {
		case SIGUSR2:
			if (info->si_pid == l_UmbrellaPid) {
				l_AllowedToWork.store(true);
			}
			break;
		case SIGINT:
		case SIGTERM:
			if (info->si_pid == l_UmbrellaPid) {
				Application::RequestShutdown();
			}
			break;
		default:
			VERIFY(!"Caught unexpected signal");
	}
}

static pid_t StartUnixWorker(const std::vector<std::string>& configs)
{
	try {
		Application::UninitializeBase();
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
			<< "Failed to stop thread pool before forking, unexpected error: " << DiagnosticInformation(ex);
		exit(EXIT_FAILURE);
	}

	(void)sigprocmask(SIG_BLOCK, &l_UnixWorkerSignals, nullptr);

	pid_t pid = fork();

	switch (pid) {
		case -1:
			Log(LogCritical, "cli")
				<< "fork() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			exit(EXIT_FAILURE);

		case 0:
			try {
				{
					struct sigaction sa;
					memset(&sa, 0, sizeof(sa));

					sa.sa_handler = SIG_DFL;

					(void)sigaction(SIGCHLD, &sa, nullptr);
					(void)sigaction(SIGHUP, &sa, nullptr);
				}

				{
					struct sigaction sa;
					memset(&sa, 0, sizeof(sa));

					sa.sa_sigaction = &WorkerSignalHandler;
					sa.sa_flags = SA_RESTART | SA_SIGINFO;

					(void)sigaction(SIGUSR2, &sa, nullptr);
					(void)sigaction(SIGINT, &sa, nullptr);
					(void)sigaction(SIGTERM, &sa, nullptr);
				}

				(void)sigprocmask(SIG_UNBLOCK, &l_UnixWorkerSignals, nullptr);

				try {
					Application::InitializeBase();
				} catch (const std::exception& ex) {
					Log(LogCritical, "cli")
						<< "Failed to re-initialize thread pool after forking (child): " << DiagnosticInformation(ex);
					_exit(EXIT_FAILURE);
				}

				_exit(RunWorker(configs));
			} catch (...) {
				_exit(EXIT_FAILURE);
			}

		default:
			l_CurrentlyStartingUnixWorkerPid.store(pid);
			(void)sigprocmask(SIG_UNBLOCK, &l_UnixWorkerSignals, nullptr);

			for (;;) {
				switch (l_CurrentlyStartingUnixWorkerState.load()) {
					case UnixWorkerState::LoadedConfig:
						break;
					case UnixWorkerState::Failed:
						while (waitpid(pid, nullptr, 0) == -1 && errno == EINTR) {
						}
						pid = -1;
						break;
					default:
						Utility::Sleep(0.2);
						continue;
				}

				break;
			}

			l_CurrentlyStartingUnixWorkerPid.store(-1);
			l_CurrentlyStartingUnixWorkerState.store(UnixWorkerState::Pending);

			try {
				Application::InitializeBase();
			} catch (const std::exception& ex) {
				Log(LogCritical, "cli")
					<< "Failed to re-initialize thread pool after forking (parent): " << DiagnosticInformation(ex);
				exit(EXIT_FAILURE);
			}
	}

	return pid;
}

class PidFileManagementApp : public Application
{
public:
	inline int Main() override
	{
		return EXIT_FAILURE;
	}
};
#endif /* _WIN32 */

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

	std::vector<std::string> configs;
	if (vm.count("config") > 0)
		configs = vm["config"].as<std::vector<std::string> >();
	else if (!vm.count("no-config")) {
		/* The implicit string assignment is needed for Windows builds. */
		String configDir = Configuration::ConfigDir;
		configs.push_back(configDir + "/icinga2.conf");
	}

	if (vm.count("validate")) {
		Log(LogInformation, "cli", "Loading configuration file(s).");

		std::vector<ConfigItem::Ptr> newItems;

		if (!DaemonUtility::LoadConfigFiles(configs, newItems, Configuration::ObjectsPath, Configuration::VarsPath))
			return EXIT_FAILURE;

		Log(LogInformation, "cli", "Finished validating the configuration file(s).");
		return EXIT_SUCCESS;
	}

	{
		pid_t runningpid = Application::ReadPidFile(Configuration::PidPath);
		if (runningpid > 0) {
			Log(LogCritical, "cli")
				<< "Another instance of Icinga already running with PID " << runningpid;
			return EXIT_FAILURE;
		}
	}

	if (vm.count("daemonize")) {
		// this subroutine either succeeds, or logs an error
		// and terminates the process (does not return).
		Daemonize();
	}

#ifndef _WIN32
	PidFileManagementApp app;

	try {
		app.UpdatePidFile(Configuration::PidPath);
	} catch (const std::exception&) {
		Log(LogCritical, "Application")
			<< "Cannot update PID file '" << Configuration::PidPath << "'. Aborting.";
		return EXIT_FAILURE;
	}

	Defer closePidFile ([&app]() {
		app.ClosePidFile(true);
	});
#endif /* _WIN32 */

	if (vm.count("daemonize") || vm.count("close-stdio")) {
		// After disabling the console log, any further errors will go to the configured log only.
		// Let's try to make this clear and say good bye.
		Log(LogInformation, "cli", "Closing console log.");

		String errorLog;
		if (vm.count("errorlog"))
			errorLog = vm["errorlog"].as<std::string>();

		CloseStdIO(errorLog);
		Logger::DisableConsoleLog();
	}

#ifdef _WIN32
	return RunWorker(configs);
#else /* _WIN32 */
	l_UmbrellaPid = getpid();
	Application::SetUmbrellaProcess(l_UmbrellaPid);

	{
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));

		sa.sa_sigaction = &UmbrellaSignalHandler;
		sa.sa_flags = SA_NOCLDSTOP | SA_RESTART | SA_SIGINFO;

		(void)sigaction(SIGCHLD, &sa, nullptr);
		(void)sigaction(SIGUSR2, &sa, nullptr);
		(void)sigaction(SIGINT, &sa, nullptr);
		(void)sigaction(SIGTERM, &sa, nullptr);
		(void)sigaction(SIGHUP, &sa, nullptr);
	}

	pid_t currentWorker = StartUnixWorker(configs);

	if (currentWorker == -1) {
		return EXIT_FAILURE;
	}

	(void)kill(currentWorker, SIGUSR2);

	bool requestedTermination = false;

	for (;;) {
		if (!requestedTermination) {
			int termSig = l_TermSignal.load();
			if (termSig != -1) {
				(void)kill(currentWorker, termSig);
				requestedTermination = true;
			}
		}

		if (l_RequestedReload.exchange(false)) {
			pid_t nextWorker = StartUnixWorker(configs);

			if (nextWorker != -1) {
				(void)kill(currentWorker, SIGTERM);
				while (waitpid(currentWorker, nullptr, 0) == -1 && errno == EINTR) {
				}

				(void)kill(nextWorker, SIGUSR2);

				currentWorker = nextWorker;
			}
		}

		{
			int status;
			if (waitpid(currentWorker, &status, WNOHANG) > 0) {
				return WIFSIGNALED(status) ? 128 + WTERMSIG(status) : WEXITSTATUS(status);
			}
		}

		Utility::Sleep(0.2);
	}
#endif /* _WIN32 */
}
