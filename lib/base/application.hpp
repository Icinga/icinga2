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

#ifndef APPLICATION_H
#define APPLICATION_H

#include "base/i2-base.hpp"
#include "base/application.thpp"
#include "base/threadpool.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include <ostream>

namespace icinga
{

/**
 * Abstract base class for applications.
 *
 * @ingroup base
 */
class Application : public ObjectImpl<Application> {
public:
	DECLARE_OBJECT(Application);

	static boost::signals2::signal<void ()> OnReopenLogs;

	~Application();

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

	static bool IsShuttingDown();

	static void SetDebuggingSeverity(LogSeverity severity);
	static LogSeverity GetDebuggingSeverity();

	void UpdatePidFile(const String& filename, pid_t pid = Utility::GetPid());
	void ClosePidFile(bool unlink);
	static pid_t ReadPidFile(const String& filename);

	static String GetExePath(const String& argv0);

	static String GetPrefixDir();
	static void DeclarePrefixDir(const String& path);

	static String GetSysconfDir();
	static void DeclareSysconfDir(const String& path);

	static String GetZonesDir();
	static void DeclareZonesDir(const String& path);

	static String GetRunDir();
	static void DeclareRunDir(const String& path);

	static String GetLocalStateDir();
	static void DeclareLocalStateDir(const String& path);

	static String GetPkgDataDir();
	static void DeclarePkgDataDir(const String& path);

	static String GetIncludeConfDir();
	static void DeclareIncludeConfDir(const String& path);

	static String GetStatePath();
	static void DeclareStatePath(const String& path);

	static String GetModAttrPath();
	static void DeclareModAttrPath(const String& path);

	static String GetObjectsPath();
	static void DeclareObjectsPath(const String& path);

	static String GetVarsPath();
	static void DeclareVarsPath(const String& path);

	static String GetPidPath();
	static void DeclarePidPath(const String& path);

	static String GetRunAsUser();
	static void DeclareRunAsUser(const String& user);

	static String GetRunAsGroup();
	static void DeclareRunAsGroup(const String& group);

#ifdef _WIN32
	static bool IsProcessElevated();
#endif /* _WIN32 */

	static int GetRLimitFiles();
	static int GetDefaultRLimitFiles();
	static void DeclareRLimitFiles(int limit);

	static int GetRLimitProcesses();
	static int GetDefaultRLimitProcesses();
	static void DeclareRLimitProcesses(int limit);

	static int GetRLimitStack();
	static int GetDefaultRLimitStack();
	static void DeclareRLimitStack(int limit);

	static int GetConcurrency();
	static void DeclareConcurrency(int ncpus);

	static ThreadPool& GetTP();

	static String GetAppVersion();
	static String GetAppSpecVersion();

	static double GetStartTime();
	static void SetStartTime(double ts);

	static double GetMainTime();
	static void SetMainTime(double ts);

	static bool GetScriptDebuggerEnabled();
	static void SetScriptDebuggerEnabled(bool enabled);

	static double GetLastReloadFailed();
	static void SetLastReloadFailed(double ts);

	static void DisplayInfoMessage(std::ostream& os, bool skipVersion = false);

protected:
	virtual void OnConfigLoaded() override;
	virtual void Stop(bool runtimeRemoved) override;

	void RunEventLoop();

	pid_t StartReloadProcess();

	virtual void OnShutdown();

	virtual void ValidateName(const String& value, const ValidationUtils& utils) override final;

private:
	static Application::Ptr m_Instance; /**< The application instance. */

	static bool m_ShuttingDown; /**< Whether the application is in the process of shutting down. */
	static bool m_RequestRestart; /**< A restart was requested through SIGHUP */
	static pid_t m_ReloadProcess; /**< The PID of a subprocess doing a reload, only valid when l_Restarting==true */
	static bool m_RequestReopenLogs; /**< Whether we should re-open log files. */

	static int m_ArgC; /**< The number of command-line arguments. */
	static char **m_ArgV; /**< Command-line arguments. */
	FILE *m_PidFile; /**< The PID file */
	static bool m_Debugging; /**< Whether debugging is enabled. */
	static LogSeverity m_DebuggingSeverity; /**< Whether debugging severity is set. */
	static double m_StartTime;
	static double m_MainTime;
	static bool m_ScriptDebuggerEnabled;
	static double m_LastReloadFailed;

#ifndef _WIN32
	static void SigIntTermHandler(int signum);
#else /* _WIN32 */
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
