// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cli/variablegetcommand.hpp"
#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/configobject.hpp"
#include "base/configtype.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include "base/scriptglobal.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("variable/get", VariableGetCommand);

String VariableGetCommand::GetDescription() const
{
	return "Prints the value of an Icinga 2 variable.";
}

String VariableGetCommand::GetShortDescription() const
{
	return "gets a variable";
}

void VariableGetCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	[[maybe_unused]] boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("current", "Uses the current value (i.e. from the running process, rather than from the vars file)");
}

int VariableGetCommand::GetMinArguments() const
{
	return 1;
}

/**
 * The entry point for the "variable get" CLI command.
 *
 * @returns An exit status.
 */
int VariableGetCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (vm.count("current")) {
		std::cout << ScriptGlobal::Get(ap[0], &Empty) << "\n";
		return 0;
	}

	String varsfile = Configuration::VarsPath;

	if (!Utility::PathExists(varsfile)) {
		Log(LogCritical, "cli")
			<< "Cannot open variables file '" << varsfile << "'.";
		Log(LogCritical, "cli", "Run 'icinga2 daemon -C' to validate config and generate the cache file.");
		return 1;
	}

	Value value = VariableUtility::GetVariable(ap[0]);

	std::cout << value << "\n";

	return 0;
}
