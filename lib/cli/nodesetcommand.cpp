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

#include "cli/nodesetcommand.hpp"
#include "cli/nodeutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("node/set", NodeSetCommand);

String NodeSetCommand::GetDescription(void) const
{
	return "Set node attribute(s).";
}

String NodeSetCommand::GetShortDescription(void) const
{
	return "set node attributes";
}

void NodeSetCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("host", po::value<std::string>(), "Icinga 2 host")
		("port", po::value<std::string>(), "Icinga 2 port")
		("log_duration", po::value<double>(), "Log duration (in seconds)");
}

int NodeSetCommand::GetMinArguments(void) const
{
	return 1;
}

bool NodeSetCommand::IsDeprecated(void) const
{
	return true;
}

/**
 * The entry point for the "node set" CLI command.
 *
 * @returns An exit status.
 */
int NodeSetCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String repoFile = NodeUtility::GetNodeRepositoryFile(ap[0]);

	if (!Utility::PathExists(repoFile)) {
		Log(LogCritical, "cli")
		    << "Node '" << ap[0] << "' does not exist.";
		return 1;
	}

	String host, port = "5665";
	double log_duration = 24 * 60 * 60;

	if (vm.count("host"))
		host = vm["host"].as<std::string>();

	if (vm.count("port"))
		port = vm["port"].as<std::string>();

	if (vm.count("log_duration"))
		log_duration = vm["log_duration"].as<double>();

	NodeUtility::AddNodeSettings(ap[0], host, port, log_duration);

	return 0;
}
