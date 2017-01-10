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

#include "cli/pkiutility.hpp"
#include "cli/clicommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/console.hpp"
#include "base/tlsstream.hpp"
#include "base/tcpsocket.hpp"
#include "base/json.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "remote/jsonrpc.hpp"
#include <fstream>
#include <iostream>

using namespace icinga;

String PkiUtility::GetPkiPath(void)
{
	return Application::GetSysconfDir() + "/icinga2/pki";
}

String PkiUtility::GetLocalCaPath(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/ca";
}

int PkiUtility::NewCa(void)
{
	String caDir = GetLocalCaPath();
	String caCertFile = caDir + "/ca.crt";
	String caKeyFile = caDir + "/ca.key";

	if (Utility::PathExists(caCertFile) && Utility::PathExists(caKeyFile)) {
		Log(LogCritical, "cli")
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
	char errbuf[120];

	InitializeOpenSSL();

	BIO *csrbio = BIO_new_file(csrfile.CStr(), "r");
	X509_REQ *req = PEM_read_bio_X509_REQ(csrbio, NULL, NULL, NULL);

	if (!req) {
		Log(LogCritical, "SSL")
		    << "Could not read X509 certificate request from '" << csrfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		return 1;
	}

	BIO_free(csrbio);

	boost::shared_ptr<X509> cert = CreateCertIcingaCA(X509_REQ_get_pubkey(req), X509_REQ_get_subject_name(req));

	X509_REQ_free(req);

	WriteCert(cert, certfile);

	return 0;
}

boost::shared_ptr<X509> PkiUtility::FetchCert(const String& host, const String& port)
{
	TcpSocket::Ptr client = new TcpSocket();

	try {
		client->Connect(host, port);
	} catch (const std::exception& ex) {
		Log(LogCritical, "pki")
		    << "Cannot connect to host '" << host << "' on port '" << port << "'";
		Log(LogDebug, "pki")
		    << "Cannot connect to host '" << host << "' on port '" << port << "':\n" << DiagnosticInformation(ex);
		return boost::shared_ptr<X509>();
	}

	boost::shared_ptr<SSL_CTX> sslContext;

	try {
		sslContext = MakeSSLContext();
	} catch (const std::exception& ex) {
		Log(LogCritical, "pki")
		    << "Cannot make SSL context.";
		Log(LogDebug, "pki")
		    << "Cannot make SSL context:\n"  << DiagnosticInformation(ex);
		return boost::shared_ptr<X509>();
	}

	TlsStream::Ptr stream = new TlsStream(client, host, RoleClient, sslContext);

	try {
		stream->Handshake();
	} catch (...) {

	}

	return stream->GetPeerCertificate();
}

int PkiUtility::WriteCert(const boost::shared_ptr<X509>& cert, const String& trustedfile)
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
    const String& certfile, const String& cafile, const boost::shared_ptr<X509>& trustedCert, const String& ticket)
{
	TcpSocket::Ptr client = new TcpSocket();

	try {
		client->Connect(host, port);
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
		    << "Cannot connect to host '" << host << "' on port '" << port << "'";
		Log(LogDebug, "cli")
		    << "Cannot connect to host '" << host << "' on port '" << port << "':\n" << DiagnosticInformation(ex);
		return 1;
	}

	boost::shared_ptr<SSL_CTX> sslContext;

	try {
		sslContext = MakeSSLContext(certfile, keyfile);
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli")
		    << "Cannot make SSL context for cert path: '" << certfile << "' key path: '" << keyfile << "' ca path: '" << cafile << "'.";
		Log(LogDebug, "cli")
		    << "Cannot make SSL context for cert path: '" << certfile << "' key path: '" << keyfile << "' ca path: '" << cafile << "':\n"  << DiagnosticInformation(ex);
		return 1;
	}

	TlsStream::Ptr stream = new TlsStream(client, host, RoleClient, sslContext);

	try {
		stream->Handshake();
	} catch (const std::exception&) {
		Log(LogCritical, "cli", "Client TLS handshake failed.");
		return 1;
	}

	boost::shared_ptr<X509> peerCert = stream->GetPeerCertificate();

	if (X509_cmp(peerCert.get(), trustedCert.get())) {
		Log(LogCritical, "cli", "Peer certificate does not match trusted certificate.");
		return 1;
	}

	Dictionary::Ptr request = new Dictionary();

	String msgid = Utility::NewUniqueID();

	request->Set("jsonrpc", "2.0");
	request->Set("id", msgid);
	request->Set("method", "pki::RequestCertificate");

	Dictionary::Ptr params = new Dictionary();
	params->Set("ticket", String(ticket));

	request->Set("params", params);

	JsonRpc::SendMessage(stream, request);

	String jsonString;
	Dictionary::Ptr response;
	StreamReadContext src;

	for (;;) {
		StreamReadStatus srs = JsonRpc::ReadMessage(stream, &jsonString, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		response = JsonRpc::DecodeMessage(jsonString);

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

	if (!response) {
		Log(LogCritical, "cli", "Could not fetch valid response. Please check the master log.");
		return 1;
	}

	Dictionary::Ptr result = response->Get("result");

	if (result->Contains("error")) {
		Log(LogCritical, "cli", result->Get("error"));
		return 1;
	}

	std::ofstream fpcert;
	fpcert.open(certfile.CStr());
	fpcert << result->Get("cert");
	fpcert.close();

	if (fpcert.fail()) {
		Log(LogCritical, "cli")
		    << "Could not write certificate to file '" << certfile << "'.";
		return 1;
	}

	Log(LogInformation, "cli")
	    << "Writing signed certificate to file '" << certfile << "'.";

	std::ofstream fpca;
	fpca.open(cafile.CStr());
	fpca << result->Get("ca");
	fpca.close();

	if (fpca.fail()) {
		Log(LogCritical, "cli")
		    << "Could not open CA certificate file '" << cafile << "' for writing.";
		return 1;
	}

	Log(LogInformation, "cli")
	    << "Writing CA certificate to file '" << cafile << "'.";

	return 0;
}

String PkiUtility::GetCertificateInformation(const boost::shared_ptr<X509>& cert) {
	BIO *out = BIO_new(BIO_s_mem());
	String pre;

	pre = "\n Subject:     ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	X509_NAME_print_ex(out, X509_get_subject_name(cert.get()), 0, XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB);

	pre = "\n Issuer:      ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	X509_NAME_print_ex(out, X509_get_issuer_name(cert.get()), 0, XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB);

	pre = "\n Valid From:  ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	ASN1_TIME_print(out, X509_get_notBefore(cert.get()));

	pre = "\n Valid Until: ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	ASN1_TIME_print(out, X509_get_notAfter(cert.get()));

	pre = "\n Fingerprint: ";
	BIO_write(out, pre.CStr(), pre.GetLength());
	unsigned char md[EVP_MAX_MD_SIZE];
	unsigned int diglen;
	X509_digest(cert.get(), EVP_sha1(), md, &diglen);

	char *data;
	long length = BIO_get_mem_data(out, &data);

	std::stringstream info;
	info << String(data, data + length);
	for (unsigned int i = 0; i < diglen; i++) {
		info << std::setfill('0') << std::setw(2) << std::uppercase
		    << std::hex << static_cast<int>(md[i]) << ' ';
	}
	info << '\n';

	return info.str();
}
