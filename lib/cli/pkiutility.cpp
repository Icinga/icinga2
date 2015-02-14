/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
	String cadir = GetLocalCaPath();

	if (Utility::PathExists(cadir)) {
		Log(LogCritical, "cli")
		    << "CA directory '" << cadir << "' already exists.";
		return 1;
	}

	if (!Utility::MkDirP(cadir, 0700)) {
		Log(LogCritical, "base")
		    << "Could not create CA directory '" << cadir << "'.";
		return 1;
	}

	MakeX509CSR("Icinga CA", cadir + "/ca.key", String(), cadir + "/ca.crt", true);

	String serialpath = cadir + "/serial.txt";

	Log(LogInformation, "cli")
	    << "Initializing serial file in '" << serialpath << "'.";

	std::ofstream fp;
	fp.open(serialpath.CStr());
	fp << "01";
	fp.close();

	if (fp.fail()) {
		Log(LogCritical, "cli")
		    << "Could not create serial file '" << serialpath << "'";
		return 1;
	}

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
	std::stringstream msgbuf;
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

	std::ofstream fpcert;
	fpcert.open(certfile.CStr());

	if (!fpcert) {
		Log(LogCritical, "cli")
		    << "Failed to open certificate file '" << certfile << "' for output";
		return 1;
	}

	fpcert << CertificateToString(cert);
	fpcert.close();

	return 0;
}

int PkiUtility::SaveCert(const String& host, const String& port, const String& keyfile, const String& certfile, const String& trustedfile)
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
		    << "Cannot make SSL context for cert path: '" << certfile << "' key path: '" << keyfile << "'.";
		Log(LogDebug, "cli")
		    << "Cannot make SSL context for cert path: '" << certfile << "' key path: '" << keyfile << "':\n"  << DiagnosticInformation(ex);
		return 1;
	}

	TlsStream::Ptr stream = new TlsStream(client, RoleClient, sslContext);

	try {
		stream->Handshake();
	} catch (...) {

	}

	boost::shared_ptr<X509> cert = stream->GetPeerCertificate();

	if (!cert) {
		Log(LogCritical, "cli", "Peer did not present a valid certificate.");
		return 1;
	}

	std::ofstream fpcert;
	fpcert.open(trustedfile.CStr());
	fpcert << CertificateToString(cert);
	fpcert.close();

	if (fpcert.fail()) {
		Log(LogCritical, "cli")
		    << "Could not write certificate to file '" << trustedfile << "'.";
		return 1;
	}

	Log(LogInformation, "cli")
	    << "Writing trusted certificate to file '" << trustedfile << "'.";

	return 0;
}

int PkiUtility::GenTicket(const String& cn, const String& salt, std::ostream& ticketfp)
{
	ticketfp << PBKDF2_SHA1(cn, salt, 50000) << "\n";

	return 0;
}

int PkiUtility::RequestCertificate(const String& host, const String& port, const String& keyfile,
    const String& certfile, const String& cafile, const String& trustedfile, const String& ticket)
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

	TlsStream::Ptr stream = new TlsStream(client, RoleClient, sslContext);

	try {
		stream->Handshake();
	} catch (const std::exception&) {
		Log(LogCritical, "cli", "Client TLS handshake failed.");
		return 1;
	}

	boost::shared_ptr<X509> peerCert = stream->GetPeerCertificate();

	boost::shared_ptr<X509> trustedCert;

	try {
		trustedCert = GetX509Certificate(trustedfile);
	} catch (const std::exception&) {
		Log(LogCritical, "cli")
		    << "Cannot get trusted from cert path: '" << trustedfile << "'.";
		return 1;
	}

	if (CertificateToString(peerCert) != CertificateToString(trustedCert)) {
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

	Dictionary::Ptr response;
	StreamReadContext src;

	for (;;) {
		StreamReadStatus srs = JsonRpc::ReadMessage(stream, &response, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

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
