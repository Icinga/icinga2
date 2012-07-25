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
	static string GetTypeName(const type_info& ti);

	static void Daemonize(void);

	static shared_ptr<SSL_CTX> MakeSSLContext(string pubkey, string privkey, string cakey);
	static string GetCertificateCN(const shared_ptr<X509>& certificate);
	static shared_ptr<X509> GetX509Certificate(string pemfile);

	static bool Match(string pattern, string text);

	static string DirName(const string& path);
	static string BaseName(const string& path);

	static void NullDeleter(void *obj);

	static double GetTime(void);

private:
	static bool m_SSLInitialized;

	Utility(void);

	static void InitializeOpenSSL(void);
};

}

#endif /* UTILITY_H */
