/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/i2-base.h"
#include "base/application.th"
#include "base/threadpool.h"
#include "base/dynamicobject.h"

namespace icinga {

class Component;

/**
 * Abstract base class for applications.
 *
 * @ingroup base
 */
class I2_BASE_API Application : public ObjectImpl<Application> {
public:
	DECLARE_PTR_TYPEDEFS(Application);

	~Application(void);

	static Application::Ptr GetInstance(void);

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

	static void SetDebugging(bool debug);
	static bool IsDebugging(void);

	void UpdatePidFile(const String& filename);
	void ClosePidFile(void);

	static String GetExePath(const String& argv0);

	static String GetPrefixDir(void);
	static void DeclarePrefixDir(const String& path);

	static String GetSysconfDir(void);
	static void DeclareSysconfDir(const String& path);

	static String GetLocalStateDir(void);
	static void DeclareLocalStateDir(const String& path);

	static String GetPkgDataDir(void);
	static void DeclarePkgDataDir(const String& path);

	static String GetStatePath(void);
	static void DeclareStatePath(const String& path);

	static String GetPidPath(void);
	static void DeclarePidPath(const String& path);

	static String GetApplicationType(void);
	static void DeclareApplicationType(const String& type);

	static void MakeVariablesConstant(void);

	static ThreadPool& GetTP(void);

	static String GetVersion(void);

	static double GetStartTime(void);
	static void SetStartTime(double ts);

protected:
	virtual void OnConfigLoaded(void);
	virtual void Stop(void);

	void RunEventLoop(void) const;

	virtual void OnShutdown(void);

private:
	static Application *m_Instance; /**< The application instance. */

	static bool m_ShuttingDown; /**< Whether the application is in the process of
				  shutting down. */
	static bool m_Restarting;
	static int m_ArgC; /**< The number of command-line arguments. */
	static char **m_ArgV; /**< Command-line arguments. */
	FILE *m_PidFile; /**< The PID file */
	static bool m_Debugging; /**< Whether debugging is enabled. */
	static double m_StartTime;

#ifndef _WIN32
	static void SigIntTermHandler(int signum);
#else /* _WIN32 */
	static BOOL WINAPI CtrlHandler(DWORD type);
	static LONG WINAPI SEHUnhandledExceptionFilter(PEXCEPTION_POINTERS exi);
#endif /* _WIN32 */

	static void DisplayVersionMessage(void);
	static void DisplayBugMessage(void);

	static void SigAbrtHandler(int signum);
	static void ExceptionHandler(void);

	static void TimeWatchThreadProc(void);
	static void NewTxTimerHandler(void);
};

}

#endif /* APPLICATION_H */
