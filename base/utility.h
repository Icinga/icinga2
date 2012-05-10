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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef UTILITY_H
#define UTILITY_H

namespace icinga
{

/**
 * Utility
 *
 * Utility functions.
 */
class I2_BASE_API Utility
{
private:
	static bool m_SSLInitialized;

	Utility(void);

	static void InitializeOpenSSL(void);

public:
	/**
	 * GetTypeName
	 *
	 * Returns the type name of an object (using RTTI).
	 */
	template<class T>
	static string GetTypeName(const T& value)
	{
		string klass = typeid(value).name();

#ifdef HAVE_GCC_ABI_DEMANGLE
		int status;
		char *realname = abi::__cxa_demangle(klass.c_str(), 0, 0, &status);

		if (realname != NULL) {
			klass = string(realname);
			free(realname);
		}
#endif /* HAVE_GCC_ABI_DEMANGLE */

		return klass;
	}

	static void Daemonize(void);

	static shared_ptr<SSL_CTX> MakeSSLContext(string pubkey, string privkey, string cakey);
	static string GetCertificateCN(const shared_ptr<X509>& certificate);
	static shared_ptr<X509> GetX509Certificate(string pemfile);

	static bool Match(string pattern, string text);
};

}

#endif /* UTILITY_H */
