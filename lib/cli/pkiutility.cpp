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

#include "cli/pkiutility.hpp"
#include "cli/clicommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/tlsstream.hpp"
#include "base/tcpsocket.hpp"
#include "base/utility.hpp"
#include "remote/jsonrpc.hpp"
#include <fstream>
#include <iostream>

using namespace icinga;

int PkiUtility::NewCa(void)
{
	String cadir = Application::GetLocalStateDir() + "/lib/icinga2/ca";

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

	shared_ptr<X509> cert = CreateCertIcingaCA(X509_REQ_get_pubkey(req), X509_REQ_get_subject_name(req));

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
	TcpSocket::Ptr client = make_shared<TcpSocket>();

	client->Connect(host, port);

	shared_ptr<SSL_CTX> sslContext = MakeSSLContext(certfile, keyfile);

	TlsStream::Ptr stream = make_shared<TlsStream>(client, RoleClient, sslContext);

	try {
		stream->Handshake();
	} catch (...) {

	}

	shared_ptr<X509> cert = stream->GetPeerCertificate();

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
	TcpSocket::Ptr client = make_shared<TcpSocket>();

	client->Connect(host, port);

	shared_ptr<SSL_CTX> sslContext = MakeSSLContext(certfile, keyfile);

	TlsStream::Ptr stream = make_shared<TlsStream>(client, RoleClient, sslContext);

	stream->Handshake();

	shared_ptr<X509> peerCert = stream->GetPeerCertificate();
	shared_ptr<X509> trustedCert = GetX509Certificate(trustedfile);

	if (CertificateToString(peerCert) != CertificateToString(trustedCert)) {
		Log(LogCritical, "cli", "Peer certificate does not match trusted certificate.");
		return 1;
	}

	Dictionary::Ptr request = make_shared<Dictionary>();

	String msgid = Utility::NewUniqueID();

	request->Set("jsonrpc", "2.0");
	request->Set("id", msgid);
	request->Set("method", "pki::RequestCertificate");

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("ticket", String(ticket));

	request->Set("params", params);

	JsonRpc::SendMessage(stream, request);

	Dictionary::Ptr response;

	for (;;) {
		response = JsonRpc::ReadMessage(stream);

		if (response->Get("id") != msgid)
			continue;

		break;
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
	    << "Writing CA certificate to file '" << certfile << "'.";

	return 0;
}
