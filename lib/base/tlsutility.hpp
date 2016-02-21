/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include "base/string.hpp"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/comp.h>
#include <openssl/sha.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/exception/info.hpp>

namespace icinga
{

void I2_BASE_API InitializeOpenSSL(void);
boost::shared_ptr<SSL_CTX> I2_BASE_API MakeSSLContext(const String& pubkey = String(), const String& privkey = String(), const String& cakey = String());
void I2_BASE_API AddCRLToSSLContext(const boost::shared_ptr<SSL_CTX>& context, const String& crlPath);
void I2_BASE_API SetCipherListToSSLContext(const boost::shared_ptr<SSL_CTX>& context, const String& cipherList);
String I2_BASE_API GetCertificateCN(const boost::shared_ptr<X509>& certificate);
boost::shared_ptr<X509> I2_BASE_API GetX509Certificate(const String& pemfile);
int I2_BASE_API MakeX509CSR(const String& cn, const String& keyfile, const String& csrfile = String(), const String& certfile = String(), const String& serialFile = String(), bool ca = false);
boost::shared_ptr<X509> I2_BASE_API CreateCert(EVP_PKEY *pubkey, X509_NAME *subject, X509_NAME *issuer, EVP_PKEY *cakey, bool ca, const String& serialfile = String());
String I2_BASE_API GetIcingaCADir(void);
String I2_BASE_API CertificateToString(const boost::shared_ptr<X509>& cert);
boost::shared_ptr<X509> I2_BASE_API CreateCertIcingaCA(EVP_PKEY *pubkey, X509_NAME *subject);
String I2_BASE_API PBKDF2_SHA1(const String& password, const String& salt, int iterations);
String I2_BASE_API SHA256(const String& s);
String I2_BASE_API RandomString(int length);

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
	return "[errinfo_openssl_error]" + tmp.str() + "\n";
}

}

#endif /* TLSUTILITY_H */
