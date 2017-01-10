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

#include "cli/noderemovecommand.hpp"
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

REGISTER_CLICOMMAND("node/remove", NodeRemoveCommand);

String NodeRemoveCommand::GetDescription(void) const
{
	return "Removes Icinga 2 node.";
}

String NodeRemoveCommand::GetShortDescription(void) const
{
	return "removes node";
}

std::vector<String> NodeRemoveCommand::GetPositionalSuggestions(const String& word) const
{
	return NodeUtility::GetNodeCompletionSuggestions(word);
}

int NodeRemoveCommand::GetMinArguments(void) const
{
	return 1;
}

int NodeRemoveCommand::GetMaxArguments(void) const
{
	return -1;
}

bool NodeRemoveCommand::IsDeprecated(void) const
{
	return true;
}

/**
 * The entry point for the "node remove" CLI command.
 *
 * @returns An exit status.
 */
int NodeRemoveCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	for (const String& node : ap) {
		NodeUtility::RemoveNode(node);
	}

	return 0;
}
