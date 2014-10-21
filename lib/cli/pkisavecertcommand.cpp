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

#include "cli/pkisavecertcommand.hpp"
#include "remote/jsonrpc.hpp"
#include "base/logger.hpp"
#include "base/tlsutility.hpp"
#include "base/tlsstream.hpp"
#include "base/tcpsocket.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/save-cert", PKISaveCertCommand);

String PKISaveCertCommand::GetDescription(void) const
{
	return "Saves another Icinga 2 instance's certificate.";
}

String PKISaveCertCommand::GetShortDescription(void) const
{
	return "saves another Icinga 2 instance's certificate";
}

void PKISaveCertCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
	    ("keyfile", po::value<std::string>(), "Key file path (input)")
	    ("certfile", po::value<std::string>(), "Certificate file path (input)")
	    ("trustedfile", po::value<std::string>(), "Trusted certificate file path (output)")
	    ("host", po::value<std::string>(), "Icinga 2 host")
	    ("port", po::value<std::string>(), "Icinga 2 port");
}

std::vector<String> PKISaveCertCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "keyfile" || argument == "certfile" || argument == "trustedfile")
		return GetBashCompletionSuggestions("file", word);
	else if (argument == "host")
		return GetBashCompletionSuggestions("hostname", word);
	else if (argument == "port")
		return GetBashCompletionSuggestions("service", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "pki save-cert" CLI command.
 *
 * @returns An exit status.
 */
int PKISaveCertCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!vm.count("host")) {
		Log(LogCritical, "cli", "Icinga 2 host (--host) must be specified.");
		return 1;
	}

	if (!vm.count("keyfile")) {
		Log(LogCritical, "cli", "Key file path (--keyfile) must be specified.");
		return 1;
	}

	if (!vm.count("certfile")) {
		Log(LogCritical, "cli", "Certificate file path (--certfile) must be specified.");
		return 1;
	}

	if (!vm.count("trustedfile")) {
		Log(LogCritical, "cli", "Trusted certificate file path (--trustedfile) must be specified.");
		return 1;
	}

	TcpSocket::Ptr client = make_shared<TcpSocket>();

	String port = "5665";

	if (vm.count("port"))
		port = vm["port"].as<std::string>();

	client->Connect(vm["host"].as<std::string>(), port);

	shared_ptr<SSL_CTX> sslContext = MakeSSLContext(vm["certfile"].as<std::string>(), vm["keyfile"].as<std::string>());

	TlsStream::Ptr stream = make_shared<TlsStream>(client, RoleClient, sslContext);

	try {
		stream->Handshake();
	} catch (...) {

	}

	shared_ptr<X509> cert = stream->GetPeerCertificate();

	String trustedfile = vm["trustedfile"].as<std::string>();

	std::ofstream fpcert;
	fpcert.open(trustedfile.CStr());
	fpcert << CertificateToString(cert);
	fpcert.close();

	if (fpcert.fail()) {
		Log(LogCritical, "cli")
		    << "Could not write certificate to file '" << trustedfile << "'.";
		return 1;
	}

	return 0;
}
