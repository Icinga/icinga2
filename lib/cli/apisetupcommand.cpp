/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/apisetupcommand.hpp"
#include "cli/apisetuputility.hpp"
#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("api/setup", ApiSetupCommand);

String ApiSetupCommand::GetDescription() const
{
	return "Setup for Icinga 2 API.";
}

String ApiSetupCommand::GetShortDescription() const
{
	return "setup for API";
}

ImpersonationLevel ApiSetupCommand::GetImpersonationLevel() const
{
	return ImpersonateRoot;
}

int ApiSetupCommand::GetMaxArguments() const
{
	return -1;
}

/**
 * The entry point for the "api setup" CLI command.
 *
 * @returns An exit status.
 */
int ApiSetupCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String cn = VariableUtility::GetVariable("NodeName");

	if (cn.IsEmpty())
		cn = Utility::GetFQDN();

	if (!ApiSetupUtility::SetupMaster(cn, true))
		return 1;

	return 0;
}
