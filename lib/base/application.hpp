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

#ifndef APPLICATION_H
#define APPLICATION_H

#include "base/i2-base.hpp"
#include "base/application.thpp"
#include "base/threadpool.hpp"
#include "base/utility.hpp"
#include "base/logger_fwd.hpp"

namespace icinga
{

/**
 * Abstract base class for applications.
 *
 * @ingroup base
 */
class I2_BASE_API Application : public ObjectImpl<Application> {
public:
	DECLARE_PTR_TYPEDEFS(Application);

	static boost::signals2::signal<void (void)> OnReopenLogs;

	~Application(void);

	static void InitializeBase(void);

	static Application::Ptr GetInstance(void);

	static void Exit(int rc);

	int Run(void);

	/**
	 * Starts the application.
	 *
	 * @returns The exit code of the application.
	 */
	virtual int Main(void) = 0;

	static void SetResourceLimits(void);

	static int GetArgC(void);
	static void SetArgC(int argc);

	static char **GetArgV(void);
	static void SetArgV(char **argv);

	static void InstallExceptionHandlers(void);

	static void RequestShutdown(void);
	static void RequestRestart(void);
	static void RequestReopenLogs(void);

	static void SetDebuggingSeverity(LogSeverity severity);
	static LogSeverity GetDebuggingSeverity(void);

	void UpdatePidFile(const String& filename, pid_t pid = Utility::GetPid());
	void ClosePidFile(bool unlink);
	static pid_t ReadPidFile(const String& filename);

	static String GetExePath(const String& argv0);

	static String GetPrefixDir(void);
	static void DeclarePrefixDir(const String& path);

	static String GetSysconfDir(void);
	static void DeclareSysconfDir(const String& path);

	static String GetZonesDir(void);
	static void DeclareZonesDir(const String& path);

	static String GetRunDir(void);
	static void DeclareRunDir(const String& path);

	static String GetLocalStateDir(void);
	static void DeclareLocalStateDir(const String& path);

	static String GetPkgDataDir(void);
	static void DeclarePkgDataDir(const String& path);

	static String GetIncludeConfDir(void);
	static void DeclareIncludeConfDir(const String& path);

	static String GetStatePath(void);
	static void DeclareStatePath(const String& path);

	static String GetObjectsPath(void);
	static void DeclareObjectsPath(const String& path);

	static String GetPidPath(void);
	static void DeclarePidPath(const String& path);

	static String GetApplicationType(void);
	static void DeclareApplicationType(const String& type);

	static void MakeVariablesConstant(void);

	static ThreadPool& GetTP(void);

	static String GetVersion(void);

	static double GetStartTime(void);
	static void SetStartTime(double ts);

    static int GetReloadTimeout(void);
    static void SetReloadTimeout(int timeout_seconds);

	static void DisplayInfoMessage(bool skipVersion = false);

protected:
	virtual void OnConfigLoaded(void);
	virtual void Stop(void);

	void RunEventLoop(void);

	pid_t StartReloadProcess(void);

	virtual void OnShutdown(void);

private:
	static Application *m_Instance; /**< The application instance. */

	static bool m_ShuttingDown; /**< Whether the application is in the process of
				  shutting down. */
	static bool m_RequestRestart; /**< A restart was requested through SIGHUP */
	static pid_t m_ReloadProcess; /**< The PID of a subprocess doing a reload, 
									only valid when l_Restarting==true */
	static bool m_RequestReopenLogs; /**< Whether we should re-open log files. */

	static int m_ArgC; /**< The number of command-line arguments. */
	static char **m_ArgV; /**< Command-line arguments. */
	FILE *m_PidFile; /**< The PID file */
	static bool m_Debugging; /**< Whether debugging is enabled. */
	static LogSeverity m_DebuggingSeverity; /**< Whether debugging severity is set. */
	static double m_StartTime;
	static int m_ReloadTimeout;

#ifndef _WIN32
	static void SigIntTermHandler(int signum);
#else /* _WIN32 */
	static BOOL WINAPI CtrlHandler(DWORD type);
	static LONG WINAPI SEHUnhandledExceptionFilter(PEXCEPTION_POINTERS exi);
#endif /* _WIN32 */

	static void DisplayBugMessage(void);

	static void SigAbrtHandler(int signum);
	static void SigUsr1Handler(int signum);
	static void ExceptionHandler(void);
};

}

#endif /* APPLICATION_H */
