/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/pkiutility.hpp"
#include "remote/apilistener.hpp"
#include "base/defer.hpp"
#include "base/io-engine.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsutility.hpp"
#include "base/console.hpp"
#include "base/tlsstream.hpp"
#include "base/tcpsocket.hpp"
#include "base/json.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "remote/jsonrpc.hpp"
#include <fstream>
#include <iostream>
#include <boost/asio/ssl/context.hpp>
#include <boost/filesystem/path.hpp>

using namespace icinga;

int PkiUtility::NewCa()
{
	String caDir = ApiListener::GetCaDir();
	String caCertFile = caDir + "/ca.crt";
	String caKeyFile = caDir + "/ca.key";

	if (Utility::PathExists(caCertFile) && Utility::PathExists(caKeyFile)) {
		Log(LogWarning, "cli")
			<< "CA files '" << caCertFile << "' and '" << caKeyFile << "' already exist.";
		return 1;
	}

	Utility::MkDirP(caDir, 0700);

	MakeX509CSR("Icinga CA", caKeyFile, String(), caCertFile, true);

	return 0;
}

int PkiUtility::NewCert(const String& cn, const String& keyfile, const String& csrfile, const String& certfile)
{
	try {
		MakeX509CSR(cn, keyfile, csrfile, certfile);
	} catch(std::exception&) {
		return 1;
	}

	return 0;
}

int PkiUtility::SignCsr(const String& csrfile, const String& certfile)
{
	char errbuf[256];

	InitializeOpenSSL();

	BIO *csrbio = BIO_new_file(csrfile.CStr(), "r");
	X509_REQ *req = PEM_read_bio_X509_REQ(csrbio, nullptr, nullptr, nullptr);

	if (!req) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Could not read X509 certificate request from '" << csrfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
		return 1;
	}

	BIO_free(csrbio);

	std::shared_ptr<EVP_PKEY> pubkey = std::shared_ptr<EVP_PKEY>(X509_REQ_get_pubkey(req), EVP_PKEY_free);
	std::shared_ptr<X509> cert = CreateCertIcingaCA(pubkey.get(), X509_REQ_get_subject_name(req));

	X509_REQ_free(req);

	WriteCert(cert, certfile);

	return 0;
}

std::shared_ptr<X509> PkiUtility::FetchCert(const String& host, const String& port)
{
	Shared<boost::asio::ssl::context>::Ptr sslContext;

	try {
		sslContext = MakeAsioSslContext();
	} catch (const std::exception& ex) {
		Log(LogCritical, "pki")
			<< "Cannot make SSL context.";
		Log(LogDebug, "pki")
			<< "Cannot make SSL context:\n"  << DiagnosticInformation(ex);
		return std::shared_ptr<X509>();
	}

	auto stream (Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *sslContext, host));

	try {
		Connect(stream->lowest_layer(), host, port);
	} catch (const std::exception& ex) {
		Log(LogCritical, "pki")
			<< "Cannot connect to host '" << host << "' on port '" << port << "'";
		Log(LogDebug, "pki")
			<< "Cannot connect to host '" << host << "' on port '" << port << "':\n" << DiagnosticInformation(ex);
		return std::shared_ptr<X509>();
	}

	auto& sslConn (stream->next_layer());

	try {
		sslConn.handshake(sslConn.client);
	} catch (const std::exception& ex) {
		Log(LogCritical, "pki")
			<< "Client TLS handshake failed. (" << ex.what() << ")";
		return std::shared_ptr<X509>();
	}

	Defer shutdown ([&sslConn]() { sslConn.shutdown(); });

	return sslConn.GetPeerCertificate();
}

int PkiUtility::WriteCert(const std::shared_ptr<X509>& cert, const String& trustedfile)
{
	std::ofstream fpcert;
	fpcert.open(trustedfile.CStr());
	fpcert << CertificateToString(cert);
	fpcert.close();

	if (fpcert.fail()) {
		Log(LogCritical, "pki")
			<< "Could not write certificate to file '" << trustedfile << "'.";
		return 1;
	}

	Log(LogInformation, "pki")
		<< "Writing certificate to file '" << trustedfile << "'.";

	return 0;
}

int PkiUtility::GenTicket(const String& cn, const String& salt, std::ostream& ticketfp)
{
	ticketfp << PBKDF2_SHA1(cn, salt, 50000) << "\n";

	return 0;
}

int PkiUtility::RequestCertificate(const String& host, const String& port, const String& keyfile,
	const String& certfile, const String& cafile, const std::shared_ptr<X509>& trustedCert, const String& ticket)
{
	Shared<boost::asio::ssl::context>::Ptr sslContext;

	try {
		sslContext = MakeAsioSslContext(certfile, keyfile);
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
			<< "Cannot make SSL context for cert path: '" << certfile << "' key path: '" << keyfile << "' ca path: '" << cafile << "'.";
		Log(LogDebug, "cli")
			<< "Cannot make SSL context for cert path: '" << certfile << "' key path: '" << keyfile << "' ca path: '" << cafile << "':\n"  << DiagnosticInformation(ex);
		return 1;
	}

	auto stream (Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *sslContext, host));

	try {
		Connect(stream->lowest_layer(), host, port);
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
			<< "Cannot connect to host '" << host << "' on port '" << port << "'";
		Log(LogDebug, "cli")
			<< "Cannot connect to host '" << host << "' on port '" << port << "':\n" << DiagnosticInformation(ex);
		return 1;
	}

	auto& sslConn (stream->next_layer());

	try {
		sslConn.handshake(sslConn.client);
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
			<< "Client TLS handshake failed: " << DiagnosticInformation(ex, false);
		return 1;
	}

	Defer shutdown ([&sslConn]() { sslConn.shutdown(); });

	auto peerCert (sslConn.GetPeerCertificate());

	if (X509_cmp(peerCert.get(), trustedCert.get())) {
		Log(LogCritical, "cli", "Peer certificate does not match trusted certificate.");
		return 1;
	}

	Dictionary::Ptr params = new Dictionary({
		{ "ticket", String(ticket) }
	});

	String msgid = Utility::NewUniqueID();

	Dictionary::Ptr request = new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "id", msgid },
		{ "method", "pki::RequestCertificate" },
		{ "params", params }
	});

	Dictionary::Ptr response;

	try {
		JsonRpc::SendMessage(stream, request);
		stream->flush();

		for (;;) {
			response = JsonRpc::DecodeMessage(JsonRpc::ReadMessage(stream));

			if (response && response->Contains("error")) {
				Log(LogCritical, "cli", "Could not fetch valid response. Please check the master log (notice or debug).");
#ifdef I2_DEBUG
				/* we shouldn't expose master errors to the user in production environments */
				Log(LogCritical, "cli", response->Get("error"));
#endif /* I2_DEBUG */
				return 1;
			}

			if (response && (response->Get("id") != msgid))
				continue;

			break;
		}
	} catch (...) {
		Log(LogCritical, "cli", "Could not fetch valid response. Please check the master log.");
		return 1;
	}

	if (!response) {
		Log(LogCritical, "cli", "Could not fetch valid response. Please check the master log.");
		return 1;
	}

	Dictionary::Ptr result = response->Get("result");

	if (result->Contains("ca")) {
		try {
			StringToCertificate(result->Get("ca"));
		} catch (const std::exception& ex) {
			Log(LogCritical, "cli")
				<< "Could not write CA file: " << DiagnosticInformation(ex, false);
			return 1;
		}

		Log(LogInformation, "cli")
			<< "Writing CA certificate to file '" << cafile << "'.";

		std::ofstream fpca;
		fpca.open(cafile.CStr());
		fpca << result->Get("ca");
		fpca.close();

		if (fpca.fail()) {
			Log(LogCritical, "cli")
				<< "Could not open CA certificate file '" << cafile << "' for writing.";
			return 1;
		}
	}

	if (result->Contains("error")) {
		LogSeverity severity;

		Value vstatus;

		if (!result->Get("status_code", &vstatus))
			vstatus = 1;

		int status = vstatus;

		if (status == 1)
			severity = LogCritical;
		else {
			severity = LogInformation;
			Log(severity, "cli", "!!!!!!");
		}

		Log(severity, "cli")
			<< "!!! " << result->Get("error");

		if (status == 1)
			return 1;
		else {
			Log(severity, "cli", "!!!!!!");
			return 0;
		}
	}

	try {
		StringToCertificate(result->Get("cert"));
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
			<< "Could not write certificate file: " << DiagnosticInformation(ex, false);
		return 1;
	}

	Log(LogInformation, "cli")
		<< "Writing signed certificate to file '" << certfile << "'.";

	std::ofstream fpcert;
	fpcert.open(certfile.CStr());
	fpcert << result->Get("cert");
	fpcert.close();

	if (fpcert.fail()) {
		Log(LogCritical, "cli")
			<< "Could not write certificate to file '" << certfile << "'.";
		return 1;
	}

	return 0;
}

String PkiUtility::GetCertificateInformation(const std::shared_ptr<X509>& cert) {
	BIO *out = BIO_new(BIO_s_mem());
	String pre;

	pre = "\n Version:             " + Convert::ToString(GetCertificateVersion(cert));
	BIO_write(out, pre.CStr(), pre.GetLength());

	pre = "\n Subject:             ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	X509_NAME_print_ex(out, X509_get_subject_name(cert.get()), 0, XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB);

	pre = "\n Issuer:              ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	X509_NAME_print_ex(out, X509_get_issuer_name(cert.get()), 0, XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB);

	pre = "\n Valid From:          ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	ASN1_TIME_print(out, X509_get_notBefore(cert.get()));

	pre = "\n Valid Until:         ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	ASN1_TIME_print(out, X509_get_notAfter(cert.get()));

	pre = "\n Serial:              ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	ASN1_INTEGER *asn1_serial = X509_get_serialNumber(cert.get());
	for (int i = 0; i < asn1_serial->length; i++) {
		BIO_printf(out, "%02x%c", asn1_serial->data[i], ((i + 1 == asn1_serial->length) ? '\n' : ':'));
	}

	pre = "\n Signature Algorithm: " + GetSignatureAlgorithm(cert);
	BIO_write(out, pre.CStr(), pre.GetLength());

	pre = "\n Subject Alt Names:   " + GetSubjectAltNames(cert)->Join(" ");
	BIO_write(out, pre.CStr(), pre.GetLength());

	pre = "\n Fingerprint:         ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	unsigned char md[EVP_MAX_MD_SIZE];
	unsigned int diglen;
	X509_digest(cert.get(), EVP_sha256(), md, &diglen);

	char *data;
	long length = BIO_get_mem_data(out, &data);

	std::stringstream info;
	info << String(data, data + length);

	BIO_free(out);

	for (unsigned int i = 0; i < diglen; i++) {
		info << std::setfill('0') << std::setw(2) << std::uppercase
			<< std::hex << static_cast<int>(md[i]) << ' ';
	}
	info << '\n';

	return info.str();
}

static void CollectRequestHandler(const Dictionary::Ptr& requests, const String& requestFile)
{
	Dictionary::Ptr request = Utility::LoadJsonFile(requestFile);

	if (!request)
		return;

	Dictionary::Ptr result = new Dictionary();

	namespace fs = boost::filesystem;
	fs::path file(requestFile.Begin(), requestFile.End());
	String fingerprint = file.stem().string();

	String certRequestText = request->Get("cert_request");
	result->Set("cert_request", certRequestText);

	Value vcertResponseText;

	if (request->Get("cert_response", &vcertResponseText)) {
		String certResponseText = vcertResponseText;
		result->Set("cert_response", certResponseText);
	}

	std::shared_ptr<X509> certRequest = StringToCertificate(certRequestText);

/* XXX (requires OpenSSL >= 1.0.0)
	time_t now;
	time(&now);
	ASN1_TIME *tm = ASN1_TIME_adj(nullptr, now, 0, 0);

	int day, sec;
	ASN1_TIME_diff(&day, &sec, tm, X509_get_notBefore(certRequest.get()));

	result->Set("timestamp",  static_cast<double>(now) + day * 24 * 60 * 60 + sec); */

	BIO *out = BIO_new(BIO_s_mem());
	ASN1_TIME_print(out, X509_get_notBefore(certRequest.get()));

	char *data;
	long length;
	length = BIO_get_mem_data(out, &data);

	result->Set("timestamp", String(data, data + length));
	BIO_free(out);

	out = BIO_new(BIO_s_mem());
	X509_NAME_print_ex(out, X509_get_subject_name(certRequest.get()), 0, XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB);

	length = BIO_get_mem_data(out, &data);

	result->Set("subject", String(data, data + length));
	BIO_free(out);

	requests->Set(fingerprint, result);
}

Dictionary::Ptr PkiUtility::GetCertificateRequests(bool removed)
{
	Dictionary::Ptr requests = new Dictionary();

	String requestDir = ApiListener::GetCertificateRequestsDir();
	String ext = "json";

	if (removed)
		ext = "removed";

	if (Utility::PathExists(requestDir))
		Utility::Glob(requestDir + "/*." + ext, [requests](const String& requestFile) { CollectRequestHandler(requests, requestFile); }, GlobFile);

	return requests;
}

