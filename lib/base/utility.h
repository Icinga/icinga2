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

#ifndef UTILITY_H
#define UTILITY_H

#include "base/i2-base.h"
#include "base/qstring.h"
#include <typeinfo>
#include <boost/function.hpp>
#include <boost/thread/tss.hpp>

namespace icinga
{

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

	static bool Glob(const String& pathSpec, const boost::function<void (const String&)>& callback);

	static void QueueAsyncCallback(const boost::function<void (void)>& callback);

	static String FormatDateTime(const char *format, double ts);

	static
#ifdef _WIN32
	HMODULE
#else /* _WIN32 */
	lt_dlhandle
#endif /* _WIN32 */
	LoadExtensionLibrary(const String& library);

#ifndef _WIN32
	static void SetNonBlocking(int fd);
	static void SetCloExec(int fd);
#endif /* _WIN32 */

	static void SetNonBlockingSocket(SOCKET s);

	static String EscapeShellCmd(const String& s);

	static void SetThreadName(const String& name);
	static String GetThreadName(void);

private:
	Utility(void);

	static boost::thread_specific_ptr<String> m_ThreadName;
};

}

#endif /* UTILITY_H */
