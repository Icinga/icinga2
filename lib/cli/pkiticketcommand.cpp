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

#include "cli/pkiticketcommand.hpp"
#include "remote/jsonrpc.hpp"
#include "base/logger.hpp"
#include "base/clicommand.hpp"
#include "base/tlsutility.hpp"
#include "base/tlsstream.hpp"
#include "base/tcpsocket.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/ticket", PKITicketCommand);

String PKITicketCommand::GetDescription(void) const
{
	return "Generates an Icinga 2 ticket";
}

String PKITicketCommand::GetShortDescription(void) const
{
	return "generates a ticket";
}

void PKITicketCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
	    ("cn", po::value<std::string>(), "Certificate common name")
	    ("salt", po::value<std::string>(), "Ticket salt");
}

/**
 * The entry point for the "pki ticket" CLI command.
 *
 * @returns An exit status.
 */
int PKITicketCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!vm.count("cn")) {
		Log(LogCritical, "cli", "Common name (--cn) must be specified.");
		return 1;
	}

	if (!vm.count("salt")) {
		Log(LogCritical, "cli", "Ticket salt (--salt) must be specified.");
		return 1;
	}

	std::cout << PBKDF2_SHA1(vm["cn"].as<std::string>(), vm["salt"].as<std::string>(), 50000) << std::endl;

	return 0;
}
