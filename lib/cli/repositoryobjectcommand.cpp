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

#include "cli/repositoryobjectcommand.hpp"
#include "cli/repositoryutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include <fstream>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_REPOSITORY_CLICOMMAND(Host);
REGISTER_REPOSITORY_CLICOMMAND(Service);
REGISTER_REPOSITORY_CLICOMMAND(Zone);
REGISTER_REPOSITORY_CLICOMMAND(Endpoint);

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
		case RepositoryCommandSet:
			description = "Set attributes for a";
			break;
		default:
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
		case RepositoryCommandSet:
			description = "set attributes for a";
			break;
		default:
			break;
	}

	description += " " + m_Type + " object";

	if (m_Command == RepositoryCommandList)
		description += "s";

	return description;
}

bool RepositoryObjectCommand::IsDeprecated(void) const
{
	return true;
}

void RepositoryObjectCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	if (m_Command == RepositoryCommandAdd) {
		visibleDesc.add_options()
			("import", po::value<std::vector<std::string> >(), "Import the defined template into the object. Must be defined and included separately in Icinga 2");
	}
}

std::vector<String> RepositoryObjectCommand::GetPositionalSuggestions(const String& word) const
{
	if (m_Command == RepositoryCommandAdd) {
		Type::Ptr ptype = Type::GetByName(m_Type);
		ASSERT(ptype);
		return GetFieldCompletionSuggestions(ptype, word);
	} else if (m_Command == RepositoryCommandRemove) {
		std::vector<String> suggestions;

		String argName = "name=";
		if (argName.Find(word) == 0)
			suggestions.push_back(argName);

		if (m_Type == "Service") {
			String argHostName = "host_name=";
			if (argHostName.Find(word) == 0)
				suggestions.push_back(argHostName);
		}

		return suggestions;
	} else
		return CLICommand::GetPositionalSuggestions(word);
}

ImpersonationLevel RepositoryObjectCommand::GetImpersonationLevel(void) const
{
	return ImpersonateRoot;
}

int RepositoryObjectCommand::GetMaxArguments(void) const
{
	return -1;
}

/**
 * The entry point for the "repository <type> <add/remove/list>" CLI command.
 *
 * @returns An exit status.
 */
int RepositoryObjectCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	Dictionary::Ptr attrs = RepositoryUtility::GetArgumentAttributes(ap);

	/* shortcut for list command */
	if (m_Command == RepositoryCommandList) {
		RepositoryUtility::PrintObjects(std::cout, m_Type);
		return 0;
	}

	if (!attrs->Contains("name")) {
		Log(LogCritical, "cli", "Object requires a name (Hint: 'name=<objectname>')!");
		return 1;
	}

	String name = attrs->Get("name");

	if (vm.count("import")) {
		Array::Ptr imports = new Array();

		for (const String& import : vm["import"].as<std::vector<std::string> >()) {
			imports->Add(import);
		}

		//Update object attributes
		if (imports->GetLength() > 0)
			attrs->Set("import", imports);
	}

	if (m_Command == RepositoryCommandAdd) {
		std::vector<String> object_paths = RepositoryUtility::GetObjects();

		Array::Ptr changes = new Array();
		RepositoryUtility::GetChangeLog(boost::bind(RepositoryUtility::CollectChange, _1, changes));

		RepositoryUtility::AddObject(object_paths, name, m_Type, attrs, changes);
	} else if (m_Command == RepositoryCommandRemove) {
		Array::Ptr changes = new Array();
		RepositoryUtility::GetChangeLog(boost::bind(RepositoryUtility::CollectChange, _1, changes));

		/* pass attrs for service->host_name requirement */
		RepositoryUtility::RemoveObject(name, m_Type, attrs, changes);
	} else if (m_Command == RepositoryCommandSet) {
		Log(LogWarning, "cli")
		    << "Not supported yet. Please check the roadmap at https://dev.icinga.com\n";
		return 1;
	} else {
		Log(LogCritical, "cli")
		    << "Invalid command '" << m_Command << "'specified.\n";
		return 1;
	}

	return 0;
}
