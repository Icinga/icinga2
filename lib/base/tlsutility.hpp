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

#ifndef TLSUTILITY_H
#define TLSUTILITY_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/qstring.hpp"
#include "base/exception.hpp"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/comp.h>
#include <openssl/sha.h>

namespace icinga
{

shared_ptr<SSL_CTX> I2_BASE_API MakeSSLContext(const String& pubkey, const String& privkey, const String& cakey);
void I2_BASE_API AddCRLToSSLContext(const shared_ptr<SSL_CTX>& context, const String& crlPath);
String I2_BASE_API GetCertificateCN(const shared_ptr<X509>& certificate);
shared_ptr<X509> I2_BASE_API GetX509Certificate(const String& pemfile);
extern "C" int I2_BASE_API MakeX509CSR(const char *cn, const char *keyfile, const char *csrfile);
String I2_BASE_API SHA256(const String& s);

class I2_BASE_API openssl_error : virtual public std::exception, virtual public boost::exception { };

struct errinfo_openssl_error_;
typedef boost::error_info<struct errinfo_openssl_error_, unsigned long> errinfo_openssl_error;

inline std::string to_string(const errinfo_openssl_error& e)
{
	std::ostringstream tmp;
	int code = e.value();
	char errbuf[120];

	const char *message = ERR_error_string(code, errbuf);

	if (message == NULL)
		message = "Unknown error.";

	tmp << code << ", \"" << message << "\"";
	return tmp.str();
}

}

#endif /* TLSUTILITY_H */
