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
#include "cli/repositoryutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_REPOSITORY_CLICOMMAND(Host);
REGISTER_REPOSITORY_CLICOMMAND(Service);
REGISTER_REPOSITORY_CLICOMMAND(Zone);
REGISTER_REPOSITORY_CLICOMMAND(Endpoint);

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

void RepositoryObjectCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("name", po::value<std::string>(), "The name of the object")
		("zone", po::value<std::string>(), "The name of the zone, e.g. the agent where this object is bound to")
		("template", po::value<std::string>(), "Import the defined template into the object. This template must be defined and included separately in Icinga 2")
		("name", po::value<std::string>(), "The name of the object");

	if (m_Type == "Service") {
		visibleDesc.add_options()
			("host", po::value<std::string>(), "The host name related to this service object");
	}
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
	if (ap.empty()) {
		Log(LogCritical, "cli")
		    << "No object name given. Bailing out.";
		return 1;
	}

	String name = ap[0];

        std::vector<String> tokens;
	Dictionary::Ptr attr = make_shared<Dictionary>();

	std::vector<std::string> attrs = ap;
	attrs.erase(attrs.begin()); //remove name

	BOOST_FOREACH(const String& kv, attrs) {
		boost::algorithm::split(tokens, kv, boost::is_any_of("="));

		if (tokens.size() == 2) {
			attr->Set(tokens[0], tokens[1]);
		} else
			Log(LogWarning, "cli")
			    << "Cannot parse passed attributes for object '" << name << "': " << boost::algorithm::join(tokens, "=");
	}

	if (vm.count("zone"))
		attr->Set("zone", String(vm["zone"].as<std::string>()));

	if (vm.count("template"))
		attr->Set("templates", String(vm["template"].as<std::string>()));

	if (m_Command == RepositoryCommandList) {
		RepositoryUtility::PrintObjects(std::cout, m_Type);
	}
	else if (m_Command == RepositoryCommandAdd) {
		RepositoryUtility::AddObject(name, m_Type, attr);
	}
	else if (m_Command == RepositoryCommandRemove) {
		RepositoryUtility::RemoveObject(name, m_Type);
	}
	else if (m_Command == RepositoryCommandSet) {
		Log(LogWarning, "cli")
		    << "Not implemented yet.\n";
		return 1;
	} else {
		Log(LogCritical, "cli")
		    << "Invalid command '" << m_Command << "'specified.\n";
		return 1;
	}

	return 0;
}
