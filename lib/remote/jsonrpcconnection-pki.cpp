/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/jsonrpcconnection.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "base/atomic-file.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include <boost/thread/once.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <openssl/asn1.h>
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
	STACK_OF(X509) *chain;

	Dictionary::Ptr result = new Dictionary();
	auto& tlsConn (origin->FromClient->GetStream()->next_layer());

	/* Use the presented client certificate if not provided. */
	if (certText.IsEmpty()) {
		cert = tlsConn.GetPeerCertificate();
		chain = tlsConn.GetPeerCertificateChain();
	} else {
		cert = StringToCertificate(certText);
		chain = nullptr;
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

	bool signedByCA = false;

	{
		Log logmsg(LogInformation, "JsonRpcConnection");
		logmsg << "Received certificate request for CN '" << cn << "'";

		try {
			signedByCA = VerifyCertificate(cacert, cert, listener->GetCrlPath(), chain);
			if (!signedByCA) {
				logmsg << " not";
			}
			logmsg << " signed by our CA.";
		} catch (const std::exception &ex) {
			logmsg << " which couldn't be verified";

			if (const unsigned long *openssl_code = boost::get_error_info<errinfo_openssl_error>(ex)) {
				logmsg << ": " << X509_verify_cert_error_string(long(*openssl_code)) << " (code " << *openssl_code << ")";
			} else {
				logmsg << ".";
			}
		}
	}

	std::shared_ptr<X509> parsedRequestorCA;
	X509* requestorCA = nullptr;

	if (signedByCA) {
		bool uptodate = IsCertUptodate(cert);

		if (uptodate) {
			// Even if the leaf is up-to-date, the root may expire soon.
			// In a regular setup where Icinga manages the PKI, there is only one CA.
			// Icinga includes it in handshakes, let's see whether the peer needs a fresh one...

			if (cn == origin->FromClient->GetIdentity()) {
				auto chain (SSL_get_peer_cert_chain(tlsConn.native_handle()));

				if (chain) {
					auto len (sk_X509_num(chain));

					for (int i = 0; i < len; ++i) {
						auto link (sk_X509_value(chain, i));

						if (!X509_NAME_cmp(X509_get_subject_name(link), X509_get_issuer_name(link))) {
							requestorCA = link;
						}
					}
				}
			} else {
				Value requestorCaStr;

				if (params->Get("requestor_ca", &requestorCaStr)) {
					parsedRequestorCA = StringToCertificate(requestorCaStr);
					requestorCA = parsedRequestorCA.get();
				}
			}

			if (requestorCA && !IsCaUptodate(requestorCA)) {
				int days;

				if (ASN1_TIME_diff(&days, nullptr, X509_get_notAfter(requestorCA), X509_get_notAfter(cacert.get())) && days > 0) {
					uptodate = false;
				}
			}
		}

		if (uptodate) {
			Log(LogInformation, "JsonRpcConnection")
				<< "The certificates for CN '" << cn << "' and its root CA are valid and uptodate. Skipping automated renewal.";
			result->Set("status_code", 1);
			result->Set("error", "The certificates for CN '" + cn + "' and its root CA are valid and uptodate. Skipping automated renewal.");
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
	} else if (Utility::PathExists(requestDir + "/" + certFingerprint + ".removed")) {
		Log(LogInformation, "JsonRpcConnection")
			<< "Certificate for CN " << cn << " has been removed. Ignoring signing request.";
		result->Set("status_code", 1);
		result->Set("error", "Ticket for CN " + cn + " declined by administrator.");
		return result;
	}

	std::shared_ptr<X509> newcert;
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

		// Auto-signing is disabled: Client did not include a ticket in its request.
		if (ticket.IsEmpty()) {
			Log(LogNotice, "JsonRpcConnection")
				<< "Certificate request for CN '" << cn
				<< "': No ticket included, skipping auto-signing and waiting for on-demand signing approval.";

			goto delayed_request;
		}

		// Auto-signing is disabled: no TicketSalt
		if (salt.IsEmpty()) {
			Log(LogNotice, "JsonRpcConnection")
				<< "Certificate request for CN '" << cn
				<< "': This instance is the signing master for the Icinga CA."
				<< " The 'ticket_salt' attribute in the 'api' feature is not set."
				<< " Not signing the request. Please check the docs.";

			goto delayed_request;
		}

		String realTicket = PBKDF2_SHA1(cn, salt, 50000);

		Log(LogDebug, "JsonRpcConnection")
			<< "Certificate request for CN '" << cn << "': Comparing received ticket '"
			<< ticket << "' with calculated ticket '" << realTicket << "'.";

		if (!Utility::ComparePasswords(ticket, realTicket)) {
			Log(LogWarning, "JsonRpcConnection")
				<< "Ticket '" << ticket << "' for CN '" << cn << "' is invalid.";

			result->Set("status_code", 1);
			result->Set("error", "Invalid ticket for CN '" + cn + "'.");
			return result;
		}
	}

	newcert = listener->RenewCert(cert);

	if (!newcert) {
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

	if (requestorCA) {
		request->Set("requestor_ca", CertificateToString(requestorCA));
	}

	Utility::SaveJsonFile(requestPath, 0600, request);

	JsonRpcConnection::SendCertificateRequest(nullptr, origin, requestPath);

	result->Set("status_code", 2);
	result->Set("error", "Certificate request for CN '" + cn + "' is pending. Waiting for approval from the parent Icinga instance.");

	Log(LogInformation, "JsonRpcConnection")
		<< "Certificate request for CN '" << cn << "' is pending. Waiting for approval.";

	if (origin) {
		auto client (origin->FromClient);

		if (client && !client->GetEndpoint()) {
			client->Disconnect();
		}
	}

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
		{
			Log msg (LogInformation, "JsonRpcConnection");
			msg << "Requesting new certificate for this Icinga instance";

			if (aclient) {
				msg << " from endpoint '" << aclient->GetIdentity() << "'";
			}

			msg << ".";
		}

		String ticketPath = ApiListener::GetCertsDir() + "/ticket";

		std::ifstream fp(ticketPath.CStr());
		String ticket((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
		fp.close();

		params->Set("ticket", ticket);
	} else {
		Dictionary::Ptr request = Utility::LoadJsonFile(path);

		if (request->Contains("cert_response"))
			return;

		request->CopyTo(params);
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

	AtomicFile::Write(caPath, 0644, ca);

	/* Update signed certificate. */
	String certPath = listener->GetDefaultCertPath();

	Log(LogInformation, "JsonRpcConnection")
		<< "Updating client certificate for CN '" << cn << "' in '" << certPath << "'.";

	AtomicFile::Write(certPath, 0644, cert);

	/* Remove ticket for successful signing request. */
	String ticketPath = ApiListener::GetCertsDir() + "/ticket";

	Utility::Remove(ticketPath);

	/* Update the certificates at runtime and reconnect all endpoints. */
	Log(LogInformation, "JsonRpcConnection")
		<< "Updating the client certificate for CN '" << cn << "' at runtime and reconnecting the endpoints.";

	listener->UpdateSSLContext();

	return Empty;
}
