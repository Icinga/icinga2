/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/pkinewcacommand.hpp"
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_CLICOMMAND("pki/new-ca", PKINewCACommand);

String PKINewCACommand::GetDescription() const
{
	return "Sets up a new Certificate Authority.";
}

String PKINewCACommand::GetShortDescription() const
{
	return "sets up a new CA";
}

/**
 * The entry point for the "pki new-ca" CLI command.
 *
 * @returns An exit status.
 */
int PKINewCACommand::Run(const boost::program_options::variables_map&, [[maybe_unused]] const std::vector<std::string>& ap) const
{
	return PkiUtility::NewCa();
}
