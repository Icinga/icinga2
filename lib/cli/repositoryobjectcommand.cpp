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

#include "cli/repositoryobjectcommand.hpp"
#include "base/logger.hpp"
#include "base/clicommand.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_REPOSITORY_CLICOMMAND("Host");
REGISTER_REPOSITORY_CLICOMMAND("Service");
REGISTER_REPOSITORY_CLICOMMAND("Zone");
REGISTER_REPOSITORY_CLICOMMAND("Endpoint");

RegisterRepositoryCLICommandHelper::RegisterRepositoryCLICommandHelper(const String& type)
{
	String ltype = type;
	boost::algorithm::to_lower(ltype);

	std::vector<String> name;
	name.push_back("repository");
	name.push_back(ltype);
	name.push_back("add");
	CLICommand::Register(name, make_shared<RepositoryObjectCommand>(type, RepositoryCommandAdd));

	name[2] = "remove";
	CLICommand::Register(name, make_shared<RepositoryObjectCommand>(type, RepositoryCommandRemove));

	name[2] = "list";
	CLICommand::Register(name, make_shared<RepositoryObjectCommand>(type, RepositoryCommandList));
}

RepositoryObjectCommand::RepositoryObjectCommand(const String& type, RepositoryCommandType command)
	: m_Type(type), m_Command(command)
{ }

String RepositoryObjectCommand::GetDescription(void) const
{
	String description;

	switch (m_Command) {
		case RepositoryCommandAdd:
			description = "Adds a new";
			break;
		case RepositoryCommandRemove:
			description = "Removes a";
			break;
		case RepositoryCommandList:
			description = "Lists all";
			break;
	}

	description += " " + m_Type + " object";

	if (m_Command == RepositoryCommandList)
		description += "s";

	return description;
}

String RepositoryObjectCommand::GetShortDescription(void) const
{
	String description;

	switch (m_Command) {
		case RepositoryCommandAdd:
			description = "adds a new";
			break;
		case RepositoryCommandRemove:
			description = "removes a";
			break;
		case RepositoryCommandList:
			description = "lists all";
			break;
	}

	description += " " + m_Type + " object";

	if (m_Command == RepositoryCommandList)
		description += "s";

	return description;
}

void RepositoryObjectCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("name", po::value<std::string>(), "The name of the object");
}

std::vector<String> RepositoryObjectCommand::GetPositionalSuggestions(const String& word) const
{
	if (m_Command == RepositoryCommandAdd) {
		const Type *ptype = Type::GetByName(m_Type);
		ASSERT(ptype);
		return GetFieldCompletionSuggestions(ptype, word);
	} else
		return CLICommand::GetPositionalSuggestions(word);
}

/**
 * The entry point for the "repository <type> <add/remove/list>" CLI command.
 *
 * @returns An exit status.
 */
int RepositoryObjectCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	return 0;
}
