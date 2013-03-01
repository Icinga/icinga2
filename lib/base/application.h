/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

namespace icinga {

class Component;

/**
 * Abstract base class for applications.
 *
 * @ingroup base
 */
class I2_BASE_API Application : public DynamicObject {
public:
	typedef shared_ptr<Application> Ptr;
	typedef weak_ptr<Application> WeakPtr;

	Application(const Dictionary::Ptr& serializedUpdate);
	~Application(void);

	static Application *GetInstance(void);

	int Run(void);

	/**
	 * Starts the application.
	 *
	 * @returns The exit code of the application.
	 */
	virtual int Main(void) = 0;

	static int GetArgC(void);
	static void SetArgC(int argc);

	static char **GetArgV(void);
	static void SetArgV(char **argv);

	static void InstallExceptionHandlers(void);

	static void RequestShutdown(void);

	static void SetDebugging(bool debug);
	static bool IsDebugging(void);

	void UpdatePidFile(const String& filename);
	void ClosePidFile(void);

	static String GetExePath(const String& argv0);

	static String GetPrefixDir(void);
	static void SetPrefixDir(const String& path);

	static String GetLocalStateDir(void);
	static void SetLocalStateDir(const String& path);

	static String GetPkgLibDir(void);
	static void SetPkgLibDir(const String& path);

	static String GetPkgDataDir(void);
	static void SetPkgDataDir(const String& path);

	static EventQueue& GetEQ(void);

protected:
	void RunEventLoop(void) const;

	virtual void OnShutdown(void) = 0;

private:
	static Application *m_Instance; /**< The application instance. */

	static bool m_ShuttingDown; /**< Whether the application is in the process of
				  shutting down. */
	static int m_ArgC; /**< The number of command-line arguments. */
	static char **m_ArgV; /**< Command-line arguments. */
	FILE *m_PidFile; /**< The PID file */
	static bool m_Debugging; /**< Whether debugging is enabled. */
	static String m_PrefixDir; /**< The installation prefix. */
	static String m_LocalStateDir; /**< The local state dir. */
	static String m_PkgLibDir; /**< The package lib dir. */
	static String m_PkgDataDir; /**< The package data dir. */

#ifndef _WIN32
	static void SigIntHandler(int signum);
	static void SigAbrtHandler(int signum);
#else /* _WIN32 */
	static BOOL WINAPI CtrlHandler(DWORD type);
#endif /* _WIN32 */

	static void DisplayBugMessage(void);

	static void ExceptionHandler(void);

	static void TimeWatchThreadProc(void);
	static void NewTxTimerHandler(void);
	static void ShutdownTimerHandler(void);
};

}

#endif /* APPLICATION_H */
