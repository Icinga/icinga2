/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/logger.hpp"
#include "cli/pkiticketcommand.hpp"
#include "cli/variableutility.hpp"
#include "remote/pkiutility.hpp"
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
