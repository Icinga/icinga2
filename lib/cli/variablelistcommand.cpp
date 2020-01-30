/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/variablelistcommand.hpp"
#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/configobject.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("variable/list", VariableListCommand);

String VariableListCommand::GetDescription() const
{
	return "Lists all Icinga 2 variables.";
}

String VariableListCommand::GetShortDescription() const
{
	return "lists all variables";
}

/**
 * The entry point for the "variable list" CLI command.
 *
 * @returns An exit status.
 */
int VariableListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String varsfile = Configuration::VarsPath;

	if (!Utility::PathExists(varsfile)) {
		Log(LogCritical, "cli")
			<< "Cannot open variables file '" << varsfile << "'.";
		Log(LogCritical, "cli", "Run 'icinga2 daemon -C' to validate config and generate the cache file.");
		return 1;
	}

	VariableUtility::PrintVariables(std::cout);

	return 0;
}
