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

#include "cli/pkirequestcommand.hpp"
#include "remote/jsonrpc.hpp"
#include "base/logger.hpp"
#include "base/clicommand.hpp"
#include "base/tlsutility.hpp"
#include "base/tlsstream.hpp"
#include "base/tcpsocket.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/request", PKIRequestCommand);

String PKIRequestCommand::GetDescription(void) const
{
	return "Sends a PKI request to Icinga 2.";
}

String PKIRequestCommand::GetShortDescription(void) const
{
	return "requests a certificate";
}

void PKIRequestCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
	    ("keyfile", po::value<std::string>(), "Key file path")
	    ("certfile", po::value<std::string>(), "Certificate file path (input + output)")
	    ("cafile", po::value<std::string>(), "CA file path (output)")
	    ("host", po::value<std::string>(), "Icinga 2 host")
	    ("port", po::value<std::string>(), "Icinga 2 port")
	    ("ticket", po::value<std::string>(), "Icinga 2 PKI ticket");
}

std::vector<String> PKIRequestCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "keyfile" || argument == "certfile" || argument == "cafile")
		return GetBashCompletionSuggestions("file", word);
	else if (argument == "host")
		return GetBashCompletionSuggestions("hostname", word);
	else if (argument == "port")
		return GetBashCompletionSuggestions("service", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "pki request" CLI command.
 *
 * @returns An exit status.
 */
int PKIRequestCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
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

	if (!vm.count("cafile")) {
		Log(LogCritical, "cli", "CA certificate file path (--cafile) must be specified.");
		return 1;
	}

	if (!vm.count("ticket")) {
		Log(LogCritical, "cli", "Ticket (--ticket) must be specified.");
		return 1;
	}

	TcpSocket::Ptr client = make_shared<TcpSocket>();

	String port = "5665";

	if (vm.count("port"))
		port = vm["port"].as<std::string>();

	client->Connect(vm["host"].as<std::string>(), port);

	String certfile = vm["certfile"].as<std::string>();

	shared_ptr<SSL_CTX> sslContext = MakeSSLContext(certfile, vm["keyfile"].as<std::string>());

	TlsStream::Ptr stream = make_shared<TlsStream>(client, RoleClient, sslContext);

	stream->Handshake();

	Dictionary::Ptr request = make_shared<Dictionary>();

	String msgid = Utility::NewUniqueID();

	request->Set("jsonrpc", "2.0");
	request->Set("id", msgid);
	request->Set("method", "pki::RequestCertificate");

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("ticket", String(vm["ticket"].as<std::string>()));

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

	String cafile = vm["cafile"].as<std::string>();

	std::ofstream fpcert;
	fpcert.open(certfile.CStr());

	if (!fpcert) {
		Log(LogCritical, "cli", "Could not open certificate file '" + certfile + "' for writing.");
		return 1;
	}

	fpcert << result->Get("cert");
	fpcert.close();

	std::ofstream fpca;
	fpca.open(cafile.CStr());

	if (!fpcert) {
		Log(LogCritical, "cli", "Could not open CA certificate file '" + cafile + "' for writing.");
		return 1;
	}

	fpca << result->Get("ca");
	fpca.close();

	return 0;
}
