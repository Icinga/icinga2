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

#include "cli/calistcommand.hpp"
#include "remote/apilistener.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/json.hpp"

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("ca/list", CAListCommand);

String CAListCommand::GetDescription(void) const
{
	return "Lists all certificate signing requests.";
}

String CAListCommand::GetShortDescription(void) const
{
	return "lists all certificate signing requests";
}

void CAListCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("json", "encode output as JSON")
	;
}
static void CollectRequestHandler(const Dictionary::Ptr& requests, const String& requestFile)
{
	Dictionary::Ptr request = Utility::LoadJsonFile(requestFile);

	if (!request)
		return;

	Dictionary::Ptr result = new Dictionary();

	String fingerprint = Utility::BaseName(requestFile);
	fingerprint = fingerprint.SubStr(0, fingerprint.GetLength() - 5);

	String certRequestText = request->Get("cert_request");
	result->Set("cert_request", certRequestText);

	Value vcertResponseText;

	if (request->Get("cert_response", &vcertResponseText)) {
		String certResponseText = vcertResponseText;
		result->Set("cert_response", certResponseText);
	}

	boost::shared_ptr<X509> certRequest = StringToCertificate(certRequestText);

	time_t now;
	time(&now);
	ASN1_TIME *tm = ASN1_TIME_adj(NULL, now, 0, 0);

	int day, sec;
	ASN1_TIME_diff(&day, &sec, tm, X509_get_notBefore(certRequest.get()));

	result->Set("timestamp",  static_cast<double>(now) + day * 24 * 60 * 60 + sec);

	BIO *out = BIO_new(BIO_s_mem());
	X509_NAME_print_ex(out, X509_get_subject_name(certRequest.get()), 0, XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB);

	char *data;
	long length;
	length = BIO_get_mem_data(out, &data);

	result->Set("subject", String(data, data + length));
	BIO_free(out);

	requests->Set(fingerprint, result);
}

/**
 * The entry point for the "ca list" CLI command.
 *
 * @returns An exit status.
 */
int CAListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	Dictionary::Ptr requests = new Dictionary();

	String requestDir = ApiListener::GetPkiRequestsDir();

	if (Utility::PathExists(requestDir))
		Utility::Glob(requestDir + "/*.json", boost::bind(&CollectRequestHandler, requests, _1), GlobFile);

	if (vm.count("json"))
		std::cout << JsonEncode(requests);
	else {
		ObjectLock olock(requests);

		std::cout << "Fingerprint                                                      | Timestamp           | Signed | Subject\n";
		std::cout << "-----------------------------------------------------------------|---------------------|--------|--------\n";

		for (auto& kv : requests) {
			Dictionary::Ptr request = kv.second;

			std::cout << kv.first
			    << " | "
			    << Utility::FormatDateTime("%Y/%m/%d %H:%M:%S", request->Get("timestamp"))
			    << " | "
			    << (request->Contains("cert_response") ? "*" : " ") << "     "
			    << " | "
			    << request->Get("subject")
			    << "\n";
		}
	}

	return 0;
}
