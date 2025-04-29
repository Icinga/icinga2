/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/daemoncommand.hpp"
#include "cli/daemonutility.hpp"
#include "remote/apilistener.hpp"
#include "remote/configobjectslock.hpp"
#include "remote/configobjectutility.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "config/configitembuilder.hpp"
#include "base/atomic.hpp"
#include "base/defer.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/process.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/scriptglobal.hpp"
#include "base/context.hpp"
#include "config.h"
#include <cstdint>
#include <cstring>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else /* _WIN32 */
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif /* _WIN32 */

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif /* HAVE_SYSTEMD */

using namespace icinga;
namespace po = boost::program_options;

static po::variables_map g_AppParams;

REGISTER_CLICOMMAND("daemon", DaemonCommand);

static inline
void NotifyStatus(const char* status)
{
#ifdef HAVE_SYSTEMD
	(void)sd_notifyf(0, "STATUS=%s", status);
#endif /* HAVE_SYSTEMD */
}

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
		("dump-objects", "write icinga2.debug cache file for icinga2 object list")
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
// The PID of the Icinga umbrella process
pid_t l_UmbrellaPid = 0;

// Whether the umbrella process allowed us to continue working beyond config validation
static Atomic<bool> l_AllowedToWork (false);
#endif /* _WIN32 */

#ifdef I2_DEBUG
/**
 * Determine whether the developer wants to delay the worker process to attach a debugger to it.
 *
 * @return Internal.DebugWorkerDelay double
 */
static double GetDebugWorkerDelay()
{
	Namespace::Ptr internal = ScriptGlobal::Get("Internal", &Empty);

	Value vdebug;
	if (internal && internal->Get("DebugWorkerDelay", &vdebug))
		return Convert::ToDouble(vdebug);

	return 0.0;
}
#endif /* I2_DEBUG */

static String l_ObjectsPath;

/**
 * Do the actual work (config loading, ...)
 *
 * @param configs Files to read config from
 * @param closeConsoleLog Whether to close the console log after config loading
 * @param stderrFile Where to log errors
 *
 * @return Exit code
 */
static inline
int RunWorker(const std::vector<std::string>& configs, bool closeConsoleLog = false, const String& stderrFile = String())
{

#ifdef I2_DEBUG
	double delay = GetDebugWorkerDelay();

	if (delay > 0.0) {
		Log(LogInformation, "RunWorker")
			<< "DEBUG: Current PID: " << Utility::GetPid() << ". Sleeping for " << delay << " seconds to allow lldb/gdb -p <PID> attachment.";

		Utility::Sleep(delay);
	}
#endif /* I2_DEBUG */

	Log(LogInformation, "cli", "Loading configuration file(s).");
	NotifyStatus("Loading configuration file(s)...");

	{
		std::vector<ConfigItem::Ptr> newItems;

		if (!DaemonUtility::LoadConfigFiles(configs, newItems, l_ObjectsPath, Configuration::VarsPath)) {
			Log(LogCritical, "cli", "Config validation failed. Re-run with 'icinga2 daemon -C' after fixing the config.");
			NotifyStatus("Config validation failed.");
			return EXIT_FAILURE;
		}

#ifndef _WIN32
		Log(LogNotice, "cli")
			<< "Notifying umbrella process (PID " << l_UmbrellaPid << ") about the config loading success";

		(void)kill(l_UmbrellaPid, SIGUSR2);

		Log(LogNotice, "cli")
			<< "Waiting for the umbrella process to let us doing the actual work";

		NotifyStatus("Waiting for the umbrella process to let us doing the actual work...");

		if (closeConsoleLog) {
			CloseStdIO(stderrFile);
			Logger::DisableConsoleLog();
		}

		// The umbrella process will send this signal when the previous worker has shut down.
		Application::GetPendingSignal(SIGUSR2, true);

		Log(LogNotice, "cli")
			<< "The umbrella process let us continue";
#endif /* _WIN32 */

		NotifyStatus("Restoring the previous program state...");

		/* restore the previous program state */
		try {
			ConfigObject::RestoreObjects(Configuration::StatePath);
		} catch (const std::exception& ex) {
			Log(LogCritical, "cli")
				<< "Failed to restore state file: " << DiagnosticInformation(ex);

			NotifyStatus("Failed to restore state file.");

			return EXIT_FAILURE;
		}

		NotifyStatus("Activating config objects...");

		// activate config only after daemonization: it starts threads and that is not compatible with fork()
		if (!ConfigItem::ActivateItems(newItems, false, true, true)) {
			Log(LogCritical, "cli", "Error activating configuration.");

			NotifyStatus("Error activating configuration.");

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

	NotifyStatus("Startup finished.");

	return Application::GetInstance()->Run();
}

#ifndef _WIN32

/**
 * Block the signals from the given list.
 *
 * @param signals A std::initializer_list containing the signals to block
 */
static void BlockUnixSignals(std::initializer_list<int> signals){
	sigset_t sigSet;

	(void)sigemptyset(&sigSet);
	for (auto signal : signals) {
		(void)sigaddset(&sigSet, signal);
	}

	(void)pthread_sigmask(SIG_BLOCK, &sigSet, nullptr);
};

/**
 * Unblocks the signal and sets the handler to the given value.
 *
 * @param signal The signal to unblock and reset
 * @param handler The new handler after unblocking
 */
static void UnblockSignalAndSetHandler(int signal, void (*handler)(int))
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	(void)sigaction(signal, &sa, nullptr);

	sigset_t sigSet;
	(void)sigemptyset(&sigSet);
	(void)sigaddset(&sigSet, signal);
	(void)pthread_sigmask(SIG_UNBLOCK, &sigSet, nullptr);
}

#ifdef HAVE_SYSTEMD
// When we last notified the watchdog.
static Atomic<double> l_LastNotifiedWatchdog (0);

/**
 * Notify the watchdog if not notified during the last 2.5s.
 */
static void NotifyWatchdog()
{
	double now = Utility::GetTime();

	if (now - l_LastNotifiedWatchdog.load() >= 2.5) {
		sd_notify(0, "WATCHDOG=1");
		l_LastNotifiedWatchdog.store(now);
	}
}
#endif /* HAVE_SYSTEMD */

/**
 * Starts seamless worker process doing the actual work (config loading, ...)
 *
 * @param configs Files to read config from
 * @param closeConsoleLog Whether to close the console log after config loading
 * @param stderrFile Where to log errors
 *
 * @return The worker's PID on success, -1 on fork(2) failure, -2 if the worker couldn't load its config
 */
static pid_t StartUnixWorker(const std::vector<std::string>& configs, bool closeConsoleLog = false, const String& stderrFile = String())
{
	Log(LogNotice, "cli")
		<< "Spawning seamless worker process doing the actual work";

	try {
		Application::UninitializeBase();
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
			<< "Failed to stop thread pool before forking, unexpected error: " << DiagnosticInformation(ex);
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();

	switch (pid) {
		case -1:
			Log(LogCritical, "cli")
				<< "fork() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

			try {
				Application::InitializeBase();
			} catch (const std::exception& ex) {
				Log(LogCritical, "cli")
					<< "Failed to re-initialize thread pool after forking (parent): " << DiagnosticInformation(ex);
				exit(EXIT_FAILURE);
			}
			return -1;

		case 0:
			try {
				/* We'll leave the other signals blocked since we're going
				 * to handle them synchronously with sigtimedwait in
				 * Application::RunEventLoop().
				 */
				UnblockSignalAndSetHandler(SIGHUP, SIG_IGN);

				try {
					Application::InitializeBase();
				} catch (const std::exception& ex) {
					Log(LogCritical, "cli")
						<< "Failed to re-initialize thread pool after forking (child): " << DiagnosticInformation(ex);
					_exit(EXIT_FAILURE);
				}

				try {
					Process::InitializeSpawnHelper();
				} catch (const std::exception& ex) {
					Log(LogCritical, "cli")
						<< "Failed to initialize process spawn helper after forking (child): " << DiagnosticInformation(ex);
					_exit(EXIT_FAILURE);
				}

				_exit(RunWorker(configs, closeConsoleLog, stderrFile));
			} catch (const std::exception& ex) {
				Log(LogCritical, "cli") << "Exception in main process: " << DiagnosticInformation(ex);
				_exit(EXIT_FAILURE);
			} catch (...) {
				_exit(EXIT_FAILURE);
			}

		default:
			Log(LogNotice, "cli")
				<< "Spawned worker process (PID " << pid << "), waiting for it to load its config";

			// Wait for the newly spawned process to either load its config or fail.
			for (;;) {
#ifdef HAVE_SYSTEMD
				NotifyWatchdog();
#endif /* HAVE_SYSTEMD */

				if (waitpid(pid, nullptr, WNOHANG) > 0) {
					Log(LogNotice, "cli")
						<< "Worker process couldn't load its config";

					pid = -2;
					break;
				}

				auto sig = Application::GetPendingSignal(SIGUSR2);
				if (sig && (sig->si_pid == 0 || sig->si_pid == pid)) {
					Log(LogNotice, "cli")
						<< "Worker process successfully loaded its config";
					break;
				}

				Utility::Sleep(0.2);
			}

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

/**
 * Workaround to instantiate Application (which is abstract) in DaemonCommand#Run()
 */
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
#ifdef _WIN32
	SetConsoleOutputCP(65001);
#endif /* _WIN32 */

#ifndef _WIN32
	/* Block the signals we'd like receive synchronously via sigtimedwait.
	 * On the worker side we'll unblock SIGHUP after fork().
	 */
	BlockUnixSignals({SIGUSR1, SIGUSR2, SIGTERM, SIGINT, SIGHUP, SIGALRM});
#endif

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

	if (vm.count("dump-objects")) {
		if (!vm.count("validate")) {
			Log(LogCritical, "cli", "--dump-objects is not allowed without -C");
			return EXIT_FAILURE;
		}

		l_ObjectsPath = Configuration::ObjectsPath;
	}

	if (vm.count("validate")) {
		Log(LogInformation, "cli", "Loading configuration file(s).");

		std::vector<ConfigItem::Ptr> newItems;

		if (!DaemonUtility::LoadConfigFiles(configs, newItems, l_ObjectsPath, Configuration::VarsPath)) {
			Log(LogCritical, "cli", "Config validation failed. Re-run with 'icinga2 daemon -C' after fixing the config.");
			return EXIT_FAILURE;
		}

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
	/* The Application manages the PID file,
	 * but on *nix this process doesn't load any config
	 * so there's no central Application instance.
	 */
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

	if (vm.count("daemonize")) {
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
	try {
		return RunWorker(configs);
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")	<< "Exception in main process: " << DiagnosticInformation(ex);
		return EXIT_FAILURE;
	} catch (...) {
		return EXIT_FAILURE;
	}
#else /* _WIN32 */
	l_UmbrellaPid = getpid();
	Application::SetUmbrellaProcess(l_UmbrellaPid);

	bool closeConsoleLog = !vm.count("daemonize") && vm.count("close-stdio");

	String errorLog;
	if (vm.count("errorlog"))
		errorLog = vm["errorlog"].as<std::string>();

	// The PID of the current seamless worker
	pid_t currentWorker = StartUnixWorker(configs, closeConsoleLog, errorLog);

	if (currentWorker < 0) {
		return EXIT_FAILURE;
	}

	if (closeConsoleLog) {
		// After disabling the console log, any further errors will go to the configured log only.
		// Let's try to make this clear and say good bye.
		Log(LogInformation, "cli", "Closing console log.");

		CloseStdIO(errorLog);
		Logger::DisableConsoleLog();
	}

	// Immediately allow the first (non-reload) worker to continue working beyond config validation
	(void)kill(currentWorker, SIGUSR2);

#ifdef HAVE_SYSTEMD
	sd_notify(0, "READY=1");
#endif /* HAVE_SYSTEMD */

	// Whether we already forwarded a termination signal to the seamless worker
	bool requestedTermination = false;

	// Whether we already notified systemd about our termination
	bool notifiedTermination = false;

	for (;;) {
#ifdef HAVE_SYSTEMD
		NotifyWatchdog();
#endif /* HAVE_SYSTEMD */

		if (!requestedTermination) {
			auto sig = Application::GetPendingSignal({SIGTERM, SIGINT});
			if (sig) {
				UnblockSignalAndSetHandler(sig->si_signo, SIG_DFL);
				Log(LogCritical, "cli")
					<< "Got signal " << sig->si_signo << ", forwarding to seamless worker (PID " << currentWorker << ")";

				(void)kill(currentWorker, sig->si_signo);
				requestedTermination = true;

#ifdef HAVE_SYSTEMD
				if (!notifiedTermination) {
					notifiedTermination = true;
					sd_notify(0, "STOPPING=1");
				}
#endif /* HAVE_SYSTEMD */
			}
		}

		if (Application::GetPendingSignal(SIGHUP)) {
			Log(LogInformation, "Application")
				<< "Got reload command: Starting new instance.";

#ifdef HAVE_SYSTEMD
			sd_notify(0, "RELOADING=1");
#endif /* HAVE_SYSTEMD */

			// The old process is still active, yet.
			// Its config changes would not be visible to the new one after config load.
			ConfigObjectsExclusiveLock lock;

			pid_t nextWorker = StartUnixWorker(configs);

			switch (nextWorker) {
				case -1:
					break;
				case -2:
					Log(LogCritical, "Application", "Found error in config: reloading aborted");
					Application::SetLastReloadFailed(Utility::GetTime());
					break;
				default:
					Log(LogInformation, "Application")
						<< "Reload done, old process shutting down. Child process with PID '" << nextWorker << "' is taking over.";

					NotifyStatus("Shutting down old instance...");

					Application::SetLastReloadFailed(0);
					(void)kill(currentWorker, SIGTERM);

					{
						double start = Utility::GetTime();

						while (waitpid(currentWorker, nullptr, 0) == -1 && errno == EINTR) {
	#ifdef HAVE_SYSTEMD
							NotifyWatchdog();
	#endif /* HAVE_SYSTEMD */
						}

						Log(LogNotice, "cli")
							<< "Waited for " << Utility::FormatDuration(Utility::GetTime() - start) << " on old process to exit.";
					}

					// Old instance shut down, allow the new one to continue working beyond config validation
					(void)kill(nextWorker, SIGUSR2);

					NotifyStatus("Shut down old instance.");

					currentWorker = nextWorker;
			}

#ifdef HAVE_SYSTEMD
			sd_notify(0, "READY=1");
#endif /* HAVE_SYSTEMD */

		}

		if (Application::GetPendingSignal(SIGUSR1)) {
			Log(LogNotice, "cli")
				<< "Got signal " << SIGUSR1 << ", forwarding to seamless worker (PID " << currentWorker << ")";

			(void)kill(currentWorker, SIGUSR1);
		}

		{
			int status;
			if (waitpid(currentWorker, &status, WNOHANG) > 0) {
				Log(LogNotice, "cli")
					<< "Seamless worker (PID " << currentWorker << ") stopped, stopping as well";

#ifdef HAVE_SYSTEMD
				if (!notifiedTermination) {
					notifiedTermination = true;
					sd_notify(0, "STOPPING=1");
				}
#endif /* HAVE_SYSTEMD */

				// If killed by signal, forward it via the exit code (to be as seamless as possible)
				return WIFSIGNALED(status) ? 128 + WTERMSIG(status) : WEXITSTATUS(status);
			}
		}

		Utility::Sleep(0.2);
	}
#endif /* _WIN32 */
}
