/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "remote/jsonrpcconnection.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include <boost/thread/once.hpp>
#include <fstream>

using namespace icinga;

static Value RequestCertificateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(RequestCertificate, pki, &RequestCertificateHandler);

Value RequestCertificateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	if (!params)
		return Empty;

	String certText = params->Get("cert_request");

	boost::shared_ptr<X509> cert;

	Dictionary::Ptr result = new Dictionary();

	if (certText.IsEmpty())
		cert = origin->FromClient->GetStream()->GetPeerCertificate();
	else
		cert = StringToCertificate(certText);

	unsigned int n;
	unsigned char digest[EVP_MAX_MD_SIZE];

	if (!X509_digest(cert.get(), EVP_sha256(), digest, &n)) {
		result->Set("status_code", 1);
		result->Set("error", "Could not calculate fingerprint for the X509 certificate.");
		return result;
	}

	char certFingerprint[EVP_MAX_MD_SIZE*2+1];
	for (unsigned int i = 0; i < n; i++)
		sprintf(certFingerprint + 2 * i, "%02x", digest[i]);

	String requestDir = Application::GetLocalStateDir() + "/lib/icinga2/pki-requests";
	String requestPath = requestDir + "/" + certFingerprint + ".json";

	ApiListener::Ptr listener = ApiListener::GetInstance();

	boost::shared_ptr<X509> cacert = GetX509Certificate(listener->GetCaPath());
	result->Set("ca", CertificateToString(cacert));

	if (Utility::PathExists(requestPath)) {
		Dictionary::Ptr request = Utility::LoadJsonFile(requestPath);

		String certResponse = request->Get("cert_response");

		if (!certResponse.IsEmpty()) {
			result->Set("cert", certResponse);
			result->Set("status_code", 0);

			return result;
		}
	}

	boost::shared_ptr<X509> newcert;
	EVP_PKEY *pubkey;
	X509_NAME *subject;

	if (!Utility::PathExists(GetIcingaCADir() + "/ca.key"))
		goto delayed_request;

	if (!origin->FromClient->IsAuthenticated()) {
		String salt = listener->GetTicketSalt();

		String ticket = params->Get("ticket");

		if (salt.IsEmpty() || ticket.IsEmpty())
			goto delayed_request;

		String realTicket = PBKDF2_SHA1(origin->FromClient->GetIdentity(), salt, 50000);

		if (ticket != realTicket) {
			result->Set("status_code", 1);
			result->Set("error", "Invalid ticket.");
			return result;
		}
	}

	pubkey = X509_get_pubkey(cert.get());
	subject = X509_get_subject_name(cert.get());

	newcert = CreateCertIcingaCA(pubkey, subject);

	/* verify that the new cert matches the CA we're using for the ApiListener;
	 * this ensures that the CA we have in /var/lib/icinga2/ca matches the one
	 * we're using for cluster connections (there's no point in sending a client
	 * a certificate it wouldn't be able to use to connect to us anyway) */
	if (!VerifyCertificate(cacert, newcert)) {
		Log(LogWarning, "JsonRpcConnection")
		    << "The CA in '" << listener->GetCaPath() << "' does not match the CA which Icinga uses "
		    << "for its own cluster connections. This is most likely a configuration problem.";
		goto delayed_request;
	}

	result->Set("cert", CertificateToString(newcert));

	result->Set("status_code", 0);

	return result;

delayed_request:
	Utility::MkDirP(requestDir, 0700);

	Dictionary::Ptr request = new Dictionary();
	request->Set("cert_request", CertificateToString(cert));
	request->Set("ticket", params->Get("ticket"));

	Utility::SaveJsonFile(requestPath, 0600, request);

	result->Set("status_code", 2);
	result->Set("error", "Certificate request is pending. Waiting for approval from the parent Icinga instance.");
	return result;
}

void JsonRpcConnection::SendCertificateRequest(void)
{
	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "pki::RequestCertificate");

	String id = Utility::NewUniqueID();
	message->Set("id", id);

	Dictionary::Ptr params = new Dictionary();

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (listener)
		params->Set("ticket", listener->GetClientTicket());

	message->Set("params", params);

	RegisterCallback(id, boost::bind(&JsonRpcConnection::CertificateRequestResponseHandler, this, _1));

	JsonRpc::SendMessage(GetStream(), message);
}

void JsonRpcConnection::CertificateRequestResponseHandler(const Dictionary::Ptr& message)
{
	Log(LogWarning, "JsonRpcConnection")
	    << message->ToString();

	Dictionary::Ptr result = message->Get("result");

	if (!result)
		return;

	String ca = result->Get("ca");
	String cert = result->Get("cert");
	int status = result->Get("status_code");

	/* TODO: make sure the cert's public key matches ours */

	if (status != 0) {
		/* TODO: log error */
		return;
	}

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	String caPath = listener->GetCaPath();

	std::fstream cafp;
	String tempCaPath = Utility::CreateTempFile(caPath + ".XXXXXX", 0644, cafp);
	cafp << ca;
	cafp.close();

	if (rename(tempCaPath.CStr(), caPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempCaPath));
	}

	String certPath = listener->GetCertPath();

	std::fstream certfp;
	String tempCertPath = Utility::CreateTempFile(certPath + ".XXXXXX", 0644, certfp);
	certfp << cert;
	certfp.close();

	if (rename(tempCertPath.CStr(), certPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempCertPath));
	}

	Log(LogInformation, "JsonRpcConnection", "Updating the client certificate for the ApiListener object");
	listener->UpdateSSLContext();
}
