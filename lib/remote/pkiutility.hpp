/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef PKIUTILITY_H
#define PKIUTILITY_H

#include "remote/i2-remote.hpp"
#include "base/dictionary.hpp"
#include "base/string.hpp"
#include <openssl/x509v3.h>

namespace icinga
{

/**
 * @ingroup remote
 */
class PkiUtility
{
public:
	static int NewCa();
	static int NewCert(const String& cn, const String& keyfile, const String& csrfile, const String& certfile);
	static int SignCsr(const String& csrfile, const String& certfile);
	static std::shared_ptr<X509> FetchCert(const String& host, const String& port);
	static int WriteCert(const std::shared_ptr<X509>& cert, const String& trustedfile);
	static int GenTicket(const String& cn, const String& salt, std::ostream& ticketfp);
	static int RequestCertificate(const String& host, const String& port, const String& keyfile,
		const String& certfile, const String& cafile, const std::shared_ptr<X509>& trustedcert,
		const String& ticket = String());
	static String GetCertificateInformation(const std::shared_ptr<X509>& certificate);
	static Dictionary::Ptr GetCertificateRequests();

private:
	PkiUtility();

};

}

#endif /* PKIUTILITY_H */
