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
	static String GetTypeName(const type_info& ti);
	static bool PrintStacktrace(ostream& fp, int ignoreFrames = 0);

	static void Daemonize(void);

	static shared_ptr<SSL_CTX> MakeSSLContext(const String& pubkey, const String& privkey, const String& cakey);
	static String GetCertificateCN(const shared_ptr<X509>& certificate);
	static shared_ptr<X509> GetX509Certificate(const String& pemfile);

	static bool Match(const String& pattern, const String& text);

	static String DirName(const String& path);
	static String BaseName(const String& path);

	static void NullDeleter(void *);

	static double GetTime(void);

	static pid_t GetPid(void);

	static void Sleep(double timeout);

	static String NewUUID(void);

	static bool Glob(const String& pathSpec, const function<void (const String&)>& callback);

	static void QueueAsyncCallback(const boost::function<void (void)>& callback);

	static
#ifdef _WIN32
	HMODULE
#else /* _WIN32 */
	lt_dlhandle
#endif /* _WIN32 */
	LoadIcingaLibrary(const String& library, bool module);

#ifndef _WIN32
	static void SetNonBlocking(int fd);
	static void SetCloExec(int fd);
#endif /* _WIN32 */

	static void SetNonBlockingSocket(SOCKET s);

private:
	static bool m_SSLInitialized;

	Utility(void);

	static void InitializeOpenSSL(void);
};

}

#endif /* UTILITY_H */
