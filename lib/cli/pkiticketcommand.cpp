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

#include "cli/pkiticketcommand.hpp"
#include "remote/pkiutility.hpp"
#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/ticket", PKITicketCommand);

String PKITicketCommand::GetDescription() const
{
	return "Generates an Icinga 2 ticket";
}

String PKITicketCommand::GetShortDescription() const
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

	String salt = VariableUtility::GetVariable("TicketSalt");

	if (vm.count("salt"))
		salt = vm["salt"].as<std::string>();

	if (salt.IsEmpty()) {
		Log(LogCritical, "cli", "Ticket salt (--salt) must be specified.");
		return 1;
	}

	return PkiUtility::GenTicket(vm["cn"].as<std::string>(), salt, std::cout);
}
