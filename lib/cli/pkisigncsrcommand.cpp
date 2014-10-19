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

#include "cli/pkisigncsrcommand.hpp"
#include "base/logger.hpp"
#include "base/clicommand.hpp"
#include "base/tlsutility.hpp"
#include "base/application.hpp"
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/sign-csr", PKISignCSRCommand);

String PKISignCSRCommand::GetDescription(void) const
{
	return "Reads a Certificate Signing Request from stdin and prints a signed certificate on stdout.";
}

String PKISignCSRCommand::GetShortDescription(void) const
{
	return "signs a CSR";
}

void PKISignCSRCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
	    ("csrfile", po::value<std::string>(), "CSR file path (input)")
	    ("certfile", po::value<std::string>(), "Certificate file path (output)");
}

std::vector<String> PKISignCSRCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "csrfile" || argument == "certfile")
		return GetBashCompletionSuggestions("file", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "pki sign-csr" CLI command.
 *
 * @returns An exit status.
 */
int PKISignCSRCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!vm.count("csrfile")) {
		Log(LogCritical, "cli", "Certificate signing request file path (--csrfile) must be specified.");
		return 1;
	}

	if (!vm.count("certfile")) {
		Log(LogCritical, "cli", "Certificate file path (--certfile) must be specified.");
		return 1;
	}

	std::stringstream msgbuf;
	char errbuf[120];

	InitializeOpenSSL();

	String csrfile = vm["csrfile"].as<std::string>();

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

	String certfile = vm["certfile"].as<std::string>();

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
