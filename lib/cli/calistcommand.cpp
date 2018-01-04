/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/json.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("ca/list", CAListCommand);

String CAListCommand::GetDescription() const
{
	return "Lists all certificate signing requests.";
}

String CAListCommand::GetShortDescription() const
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

/**
 * The entry point for the "ca list" CLI command.
 *
 * @returns An exit status.
 */
int CAListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	Dictionary::Ptr requests = PkiUtility::GetCertificateRequests();

	if (vm.count("json"))
		std::cout << JsonEncode(requests);
	else {
		ObjectLock olock(requests);

		std::cout << "Fingerprint                                                      | Timestamp                | Signed | Subject\n";
		std::cout << "-----------------------------------------------------------------|--------------------------|--------|--------\n";

		for (auto& kv : requests) {
			Dictionary::Ptr request = kv.second;

			std::cout << kv.first
				<< " | "
/*			    << Utility::FormatDateTime("%Y/%m/%d %H:%M:%S", request->Get("timestamp")) */
				<< request->Get("timestamp")
				<< " | "
				<< (request->Contains("cert_response") ? "*" : " ") << "     "
				<< " | "
				<< request->Get("subject")
				<< "\n";
		}
	}

	return 0;
}
