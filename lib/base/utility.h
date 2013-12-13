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

#ifndef UTILITY_H
#define UTILITY_H

#include "base/i2-base.h"
#include "base/qstring.h"
#include <typeinfo>
#include <boost/function.hpp>
#include <boost/thread/tss.hpp>

namespace icinga
{

#ifdef _WIN32
#define MS_VC_EXCEPTION 0x406D1388

#	pragma pack(push, 8)
struct THREADNAME_INFO
{
	DWORD dwType;
	LPCSTR szName;
	DWORD dwThreadID;
	DWORD dwFlags;
};
#	pragma pack(pop)
#endif

enum GlobType
{
	GlobFile = 1,
	GlobDirectory = 2
};

/**
 * Helper functions.
 *
 * @ingroup base
 */
class I2_BASE_API Utility
{
public:
	static String DemangleSymbolName(const String& sym);
	static String GetTypeName(const std::type_info& ti);

	static bool Match(const String& pattern, const String& text);

	static String DirName(const String& path);
	static String BaseName(const String& path);

	static void NullDeleter(void *);

	static double GetTime(void);

	static pid_t GetPid(void);

	static void Sleep(double timeout);

	static String NewUniqueID(void);

	static bool Glob(const String& pathSpec, const boost::function<void (const String&)>& callback, int type = GlobFile | GlobDirectory);
	static bool GlobRecursive(const String& path, const String& pattern, const boost::function<void (const String&)>& callback, int type = GlobFile | GlobDirectory);

	static void QueueAsyncCallback(const boost::function<void (void)>& callback);

	static String FormatDateTime(const char *format, double ts);

	static
#ifdef _WIN32
	HMODULE
#else /* _WIN32 */
	void *
#endif /* _WIN32 */
	LoadExtensionLibrary(const String& library);

#ifndef _WIN32
	static void SetNonBlocking(int fd);
	static void SetCloExec(int fd);
#endif /* _WIN32 */

	static void SetNonBlockingSocket(SOCKET s);

	static String EscapeShellCmd(const String& s);

	static void SetThreadName(const String& name, bool os = true);
	static String GetThreadName(void);

	static unsigned long SDBM(const String& str, size_t len = String::NPos);

	static int CompareVersion(const String& v1, const String& v2);

	static int Random(void);

	static tm LocalTime(time_t ts);

private:
	Utility(void);

	static boost::thread_specific_ptr<String> m_ThreadName;
	static boost::thread_specific_ptr<unsigned int> m_RandSeed;
};

}

#endif /* UTILITY_H */
