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

void ApiSetupCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("cn", po::value<std::string>(), "The certificate's common name");
}

/**
 * The entry point for the "api setup" CLI command.
 *
 * @returns An exit status.
 */
int ApiSetupCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String cn;

	if (vm.count("cn")) {
		cn = vm["cn"].as<std::string>();
	} else {
		cn = VariableUtility::GetVariable("NodeName");

		if (cn.IsEmpty())
			cn = Utility::GetFQDN();
	}

	if (!ApiSetupUtility::SetupMaster(cn, true))
		return 1;

	return 0;
}
