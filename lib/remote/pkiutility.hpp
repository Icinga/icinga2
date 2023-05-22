/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "remote/i2-remote.hpp"
#include "base/exception.hpp"
#include "base/dictionary.hpp"
#include "base/string.hpp"
#include <openssl/x509v3.h>
#include <memory>

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
	static Dictionary::Ptr GetCertificateRequests(bool removed = false);

private:
	PkiUtility();

};

}
