/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
#include <boost/regex.hpp>
#include <fstream>
#include <openssl/ssl.h>
#include <openssl/x509.h>

using namespace icinga;

static Value RequestCertificateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(RequestCertificate, pki, &RequestCertificateHandler);
static Value UpdateCertificateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(UpdateCertificate, pki, &UpdateCertificateHandler);

Value RequestCertificateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	String certText = params->Get("cert_request");

	std::shared_ptr<X509> cert;

	Dictionary::Ptr result = new Dictionary();

	/* Use the presented client certificate if not provided. */
	if (certText.IsEmpty()) {
		auto stream (origin->FromClient->GetStream());
		cert = stream->next_layer().GetPeerCertificate();
	} else {
		cert = StringToCertificate(certText);
	}

	if (!cert) {
		Log(LogWarning, "JsonRpcConnection") << "No certificate or CSR received";

		result->Set("status_code", 1);
		result->Set("error", "No certificate or CSR received.");

		return result;
	}

	ApiListener::Ptr listener = ApiListener::GetInstance();
	std::shared_ptr<X509> cacert = GetX509Certificate(listener->GetDefaultCaPath());

	String cn = GetCertificateCN(cert);

	bool signedByCA = VerifyCertificate(cacert, cert);

	Log(LogInformation, "JsonRpcConnection")
		<< "Received certificate request for CN '" << cn << "'"
		<< (signedByCA ? "" : " not") << " signed by our CA.";

	if (signedByCA) {
		time_t now;
		time(&now);

		/* auto-renew all certificates which were created before 2017 to force an update of the CA,
		 * because Icinga versions older than 2.4 sometimes create certificates with an invalid
		 * serial number. */
		time_t forceRenewalEnd = 1483228800; /* January 1st, 2017 */
		time_t renewalStart = now + 30 * 24 * 60 * 60;

		if (X509_cmp_time(X509_get_notBefore(cert.get()), &forceRenewalEnd) != -1 && X509_cmp_time(X509_get_notAfter(cert.get()), &renewalStart) != -1) {

			Log(LogInformation, "JsonRpcConnection")
				<< "The certificate for CN '" << cn << "' is valid and uptodate. Skipping automated renewal.";
			result->Set("status_code", 1);
			result->Set("error", "The certificate for CN '" + cn + "' is valid and uptodate. Skipping automated renewal.");
			return result;
		}
	}

	unsigned int n;
	unsigned char digest[EVP_MAX_MD_SIZE];

	if (!X509_digest(cert.get(), EVP_sha256(), digest, &n)) {
		result->Set("status_code", 1);
		result->Set("error", "Could not calculate fingerprint for the X509 certificate for CN '" + cn + "'.");

		Log(LogWarning, "JsonRpcConnection")
			<< "Could not calculate fingerprint for the X509 certificate requested for CN '"
			<< cn << "'.";

		return result;
	}

	char certFingerprint[EVP_MAX_MD_SIZE*2+1];
	for (unsigned int i = 0; i < n; i++)
		sprintf(certFingerprint + 2 * i, "%02x", digest[i]);

	result->Set("fingerprint_request", certFingerprint);

	String requestDir = ApiListener::GetCertificateRequestsDir();
	String requestPath = requestDir + "/" + certFingerprint + ".json";

	result->Set("ca", CertificateToString(cacert));

	JsonRpcConnection::Ptr client = origin->FromClient;

	/* If we already have a signed certificate request, send it to the client. */
	if (Utility::PathExists(requestPath)) {
		Dictionary::Ptr request = Utility::LoadJsonFile(requestPath);

		String certResponse = request->Get("cert_response");

		if (!certResponse.IsEmpty()) {
			Log(LogInformation, "JsonRpcConnection")
				<< "Sending certificate response for CN '" << cn
				<< "' to endpoint '" << client->GetIdentity() << "'.";

			result->Set("cert", certResponse);
			result->Set("status_code", 0);

			Dictionary::Ptr message = new Dictionary({
				{ "jsonrpc", "2.0" },
				{ "method", "pki::UpdateCertificate" },
				{ "params", result }
			});
			client->SendMessage(message);

			return result;
		}
	}

	std::shared_ptr<X509> newcert;
	std::shared_ptr<EVP_PKEY> pubkey;
	X509_NAME *subject;
	Dictionary::Ptr message;
	String ticket;

	/* Check whether we are a signing instance or we
	 * must delay the signing request.
	 */
	if (!Utility::PathExists(GetIcingaCADir() + "/ca.key"))
		goto delayed_request;

	if (!signedByCA) {
		String salt = listener->GetTicketSalt();

		ticket = params->Get("ticket");

		/* Auto-signing is disabled by either a) no TicketSalt
		 * or b) the client did not include a ticket in its request.
		 */
		if (salt.IsEmpty() || ticket.IsEmpty())
			goto delayed_request;

		String realTicket = PBKDF2_SHA1(cn, salt, 50000);

		if (ticket != realTicket) {
			Log(LogWarning, "JsonRpcConnection")
				<< "Ticket '" << ticket << "' for CN '" << cn << "' is invalid.";

			result->Set("status_code", 1);
			result->Set("error", "Invalid ticket for CN '" + cn + "'.");
			return result;
		}
	}

	pubkey = std::shared_ptr<EVP_PKEY>(X509_get_pubkey(cert.get()), EVP_PKEY_free);
	subject = X509_get_subject_name(cert.get());

	newcert = CreateCertIcingaCA(pubkey.get(), subject);

	/* verify that the new cert matches the CA we're using for the ApiListener;
	 * this ensures that the CA we have in /var/lib/icinga2/ca matches the one
	 * we're using for cluster connections (there's no point in sending a client
	 * a certificate it wouldn't be able to use to connect to us anyway) */
	if (!VerifyCertificate(cacert, newcert)) {
		Log(LogWarning, "JsonRpcConnection")
			<< "The CA in '" << listener->GetDefaultCaPath() << "' does not match the CA which Icinga uses "
			<< "for its own cluster connections. This is most likely a configuration problem.";
		goto delayed_request;
	}

	/* Send the signed certificate update. */
	Log(LogInformation, "JsonRpcConnection")
		<< "Sending certificate response for CN '" << cn << "' to endpoint '"
		<< client->GetIdentity() << "'" << (!ticket.IsEmpty() ? " (auto-signing ticket)" : "" ) << ".";

	result->Set("cert", CertificateToString(newcert));

	result->Set("status_code", 0);

	message = new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "method", "pki::UpdateCertificate" },
		{ "params", result }
	});
	client->SendMessage(message);

	return result;

delayed_request:
	/* Send a delayed certificate signing request. */
	Utility::MkDirP(requestDir, 0700);

	Dictionary::Ptr request = new Dictionary({
		{ "cert_request", CertificateToString(cert) },
		{ "ticket", params->Get("ticket") }
	});

	Utility::SaveJsonFile(requestPath, 0600, request);

	JsonRpcConnection::SendCertificateRequest(nullptr, origin, requestPath);

	result->Set("status_code", 2);
	result->Set("error", "Certificate request for CN '" + cn + "' is pending. Waiting for approval from the parent Icinga instance.");

	Log(LogInformation, "JsonRpcConnection")
		<< "Certificate request for CN '" << cn << "' is pending. Waiting for approval.";

	return result;
}

void JsonRpcConnection::SendCertificateRequest(const JsonRpcConnection::Ptr& aclient, const MessageOrigin::Ptr& origin, const String& path)
{
	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "pki::RequestCertificate");

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Dictionary::Ptr params = new Dictionary();
	message->Set("params", params);

	/* Path is empty if this is our own request. */
	if (path.IsEmpty()) {
		String ticketPath = ApiListener::GetCertsDir() + "/ticket";

		std::ifstream fp(ticketPath.CStr());
		String ticket((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
		fp.close();

		params->Set("ticket", ticket);
	} else {
		Dictionary::Ptr request = Utility::LoadJsonFile(path);

		if (request->Contains("cert_response"))
			return;

		params->Set("cert_request", request->Get("cert_request"));
		params->Set("ticket", request->Get("ticket"));
	}

	/* Send the request to a) the connected client
	 * or b) the local zone and all parents.
	 */
	if (aclient)
		aclient->SendMessage(message);
	else
		listener->RelayMessage(origin, Zone::GetLocalZone(), message, false);
}

Value UpdateCertificateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	if (origin->FromZone && !Zone::GetLocalZone()->IsChildOf(origin->FromZone)) {
		Log(LogWarning, "ClusterEvents")
			<< "Discarding 'update certificate' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";

		return Empty;
	}

	String ca = params->Get("ca");
	String cert = params->Get("cert");

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return Empty;

	std::shared_ptr<X509> oldCert = GetX509Certificate(listener->GetDefaultCertPath());
	std::shared_ptr<X509> newCert = StringToCertificate(cert);

	String cn = GetCertificateCN(newCert);

	Log(LogInformation, "JsonRpcConnection")
		<< "Received certificate update message for CN '" << cn << "'";

	/* Check if this is a certificate update for a subordinate instance. */
	std::shared_ptr<EVP_PKEY> oldKey = std::shared_ptr<EVP_PKEY>(X509_get_pubkey(oldCert.get()), EVP_PKEY_free);
	std::shared_ptr<EVP_PKEY> newKey = std::shared_ptr<EVP_PKEY>(X509_get_pubkey(newCert.get()), EVP_PKEY_free);

	if (X509_NAME_cmp(X509_get_subject_name(oldCert.get()), X509_get_subject_name(newCert.get())) != 0 ||
		EVP_PKEY_cmp(oldKey.get(), newKey.get()) != 1) {
		String certFingerprint = params->Get("fingerprint_request");

		/* Validate the fingerprint format. */
		boost::regex expr("^[0-9a-f]+$");

		if (!boost::regex_match(certFingerprint.GetData(), expr)) {
			Log(LogWarning, "JsonRpcConnection")
				<< "Endpoint '" << origin->FromClient->GetIdentity() << "' sent an invalid certificate fingerprint: '"
				<< certFingerprint << "' for CN '" << cn << "'.";
			return Empty;
		}

		String requestDir = ApiListener::GetCertificateRequestsDir();
		String requestPath = requestDir + "/" + certFingerprint + ".json";

		/* Save the received signed certificate request to disk. */
		if (Utility::PathExists(requestPath)) {
			Log(LogInformation, "JsonRpcConnection")
				<< "Saved certificate update for CN '" << cn << "'";

			Dictionary::Ptr request = Utility::LoadJsonFile(requestPath);
			request->Set("cert_response", cert);
			Utility::SaveJsonFile(requestPath, 0644, request);
		}

		return Empty;
	}

	/* Update CA certificate. */
	String caPath = listener->GetDefaultCaPath();

	Log(LogInformation, "JsonRpcConnection")
		<< "Updating CA certificate in '" << caPath << "'.";

	std::fstream cafp;
	String tempCaPath = Utility::CreateTempFile(caPath + ".XXXXXX", 0644, cafp);
	cafp << ca;
	cafp.close();

#ifdef _WIN32
	_unlink(caPath.CStr());
#endif /* _WIN32 */

	if (rename(tempCaPath.CStr(), caPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(tempCaPath));
	}

	/* Update signed certificate. */
	String certPath = listener->GetDefaultCertPath();

	Log(LogInformation, "JsonRpcConnection")
		<< "Updating client certificate for CN '" << cn << "' in '" << certPath << "'.";

	std::fstream certfp;
	String tempCertPath = Utility::CreateTempFile(certPath + ".XXXXXX", 0644, certfp);
	certfp << cert;
	certfp.close();

#ifdef _WIN32
	_unlink(certPath.CStr());
#endif /* _WIN32 */

	if (rename(tempCertPath.CStr(), certPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(tempCertPath));
	}

	/* Remove ticket for successful signing request. */
	String ticketPath = ApiListener::GetCertsDir() + "/ticket";

	if (unlink(ticketPath.CStr()) < 0 && errno != ENOENT) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("unlink")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(ticketPath));
	}

	/* Update the certificates at runtime and reconnect all endpoints. */
	Log(LogInformation, "JsonRpcConnection")
		<< "Updating the client certificate for CN '" << cn << "' at runtime and reconnecting the endpoints.";

	listener->UpdateSSLContext();

	return Empty;
}
