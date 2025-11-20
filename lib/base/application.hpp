/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef APPLICATION_H
#define APPLICATION_H

#include "base/i2-base.hpp"
#include "base/atomic.hpp"
#include "base/application-ti.hpp"
#include "base/logger.hpp"
#include "base/configuration.hpp"
#include "base/shared-memory.hpp"
#include <cstdint>
#include <iosfwd>
#include <type_traits>

namespace icinga
{

class ThreadPool;

/**
 * Abstract base class for applications.
 *
 * @ingroup base
 */
class Application : public ObjectImpl<Application> {
public:
	DECLARE_OBJECT(Application);

	static boost::signals2::signal<void ()> OnReopenLogs;

	~Application() override;

	static void InitializeBase();
	static void UninitializeBase();

	static Application::Ptr GetInstance();

	static void Exit(int rc);

	int Run();

	/**
	 * Starts the application.
	 *
	 * @returns The exit code of the application.
	 */
	virtual int Main() = 0;

	static void SetResourceLimits();

	static int GetArgC();
	static void SetArgC(int argc);

	static char **GetArgV();
	static void SetArgV(char **argv);

	static void InstallExceptionHandlers();

	static void RequestShutdown();
	static void RequestRestart();
	static void RequestReopenLogs();

#ifndef _WIN32
	static void SetUmbrellaProcess(pid_t pid);
#endif /* _WIN32 */

	static bool IsShuttingDown();
	static bool IsRestarting();

	static void SetDebuggingSeverity(LogSeverity severity);
	static LogSeverity GetDebuggingSeverity();

	void UpdatePidFile(const String& filename);
	void UpdatePidFile(const String& filename, pid_t pid);
	void ClosePidFile(bool unlink);
	static pid_t ReadPidFile(const String& filename);

	static String GetExePath(const String& argv0);

#ifdef _WIN32
	static bool IsProcessElevated();
#endif /* _WIN32 */

	static int GetDefaultRLimitFiles();
	static int GetDefaultRLimitProcesses();
	static int GetDefaultRLimitStack();

	static double GetReloadTimeout();

	static ThreadPool& GetTP();

	static String GetAppVersion();

	static bool GetAppIncludePackageInfo();
	static String GetAppPackageVersion();
	static String GetAppPackageRevision();
	static String GetAppPackageVendor();

	static String GetAppEnvironment();
	static void SetAppEnvironment(const String& name);

	static double GetStartTime();
	static void SetStartTime(double ts);

	static double GetUptime();

	static bool GetScriptDebuggerEnabled();
	static void SetScriptDebuggerEnabled(bool enabled);

	static double GetLastReloadFailed();
	static void SetLastReloadFailed(double ts);

	static void DisplayInfoMessage(std::ostream& os, bool skipVersion = false);

protected:
	void OnConfigLoaded() override;
	void Stop(bool runtimeRemoved) override;

	void RunEventLoop();

	pid_t StartReloadProcess();

	virtual void OnShutdown();

	void ValidateName(const Lazy<String>& lvalue, const ValidationUtils& utils) override final;

private:
	static Application::Ptr m_Instance; /**< The application instance. */

	static bool m_ShuttingDown; /**< Whether the application is in the process of shutting down. */
	static bool m_RequestRestart; /**< A restart was requested through SIGHUP */
	static pid_t m_ReloadProcess; /**< The PID of a subprocess doing a reload, only valid when l_Restarting==true */
	static bool m_RequestReopenLogs; /**< Whether we should re-open log files. */

#ifndef _WIN32
	static pid_t m_UmbrellaProcess; /**< The PID of the Icinga umbrella process */
#endif /* _WIN32 */

	static int m_ArgC; /**< The number of command-line arguments. */
	static char **m_ArgV; /**< Command-line arguments. */
	FILE *m_PidFile = nullptr; /**< The PID file */
	static bool m_Debugging; /**< Whether debugging is enabled. */
	static LogSeverity m_DebuggingSeverity; /**< Whether debugging severity is set. */
	static double m_StartTime;
	static double m_MainTime;
	static bool m_ScriptDebuggerEnabled;
#ifdef _WIN32
	static double m_LastReloadFailed;
#else /* _WIN32 */
	typedef Atomic<std::conditional_t<Atomic<double>::is_always_lock_free, double, uint32_t>> AtomicTs;
	static_assert(AtomicTs::is_always_lock_free);
	static SharedMemory<AtomicTs> m_LastReloadFailed;
#endif /* _WIN32 */

#ifdef _WIN32
	static BOOL WINAPI CtrlHandler(DWORD type);
	static LONG WINAPI SEHUnhandledExceptionFilter(PEXCEPTION_POINTERS exi);
#endif /* _WIN32 */

	static void DisplayBugMessage(std::ostream& os);

	static void SigAbrtHandler(int signum);
	static void SigUsr1Handler(int signum);
	static void ExceptionHandler();

	static String GetCrashReportFilename();

	static void AttachDebugger(const String& filename, bool interactive);
};

}

#endif /* APPLICATION_H */
