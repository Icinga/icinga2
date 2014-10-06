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

#include "base/clicommand.hpp"
#include "base/logger_fwd.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <algorithm>

using namespace icinga;
namespace po = boost::program_options;

boost::mutex l_RegistryMutex;
std::map<std::vector<String>, CLICommand::Ptr> l_Registry;

CLICommand::Ptr CLICommand::GetByName(const std::vector<String>& name)
{
	boost::mutex::scoped_lock lock(l_RegistryMutex);

	std::map<std::vector<String>, CLICommand::Ptr>::const_iterator it = l_Registry.find(name);

	if (it == l_Registry.end())
		return CLICommand::Ptr();

	return it->second;
}

void CLICommand::Register(const std::vector<String>& name, const CLICommand::Ptr& function)
{
	boost::mutex::scoped_lock lock(l_RegistryMutex);
	l_Registry[name] = function;
}

void CLICommand::Unregister(const std::vector<String>& name)
{
	boost::mutex::scoped_lock lock(l_RegistryMutex);
	l_Registry.erase(name);
}

RegisterCLICommandHelper::RegisterCLICommandHelper(const String& name, const CLICommand::Ptr& command)
{
	std::vector<String> vname;
	boost::algorithm::split(vname, name, boost::is_any_of("/"));
	CLICommand::Register(vname, command);
}

bool CLICommand::ParseCommand(int argc, char **argv, po::options_description& desc, po::variables_map& vm,
    String& cmdname, CLICommand::Ptr& command, bool& autocomplete)
{
	boost::mutex::scoped_lock lock(l_RegistryMutex);

	typedef std::map<std::vector<String>, CLICommand::Ptr>::value_type CLIKeyValue;

	std::vector<String> best_match;
	int arg_end = 1;

	BOOST_FOREACH(const CLIKeyValue& kv, l_Registry) {
		const std::vector<String>& vname = kv.first;

		for (int i = 0, k = 1; i < vname.size() && k < argc; i++, k++) {
			if (strcmp(argv[k], "--no-stack-rlimit") == 0 || strcmp(argv[k], "--autocomplete") == 0) {
				if (strcmp(argv[k], "--autocomplete") == 0) {
					autocomplete = true;
					return false;
				}

				i--;
				continue;
			}

			if (vname[i] != argv[k])
				break;
			
			if (i >= best_match.size())
				best_match.push_back(vname[i]);
			
			if (i == vname.size() - 1) {
				cmdname = boost::algorithm::join(vname, " ");
				command = kv.second;
				arg_end = k;
				goto found_command;
			}
		}
	}
	
found_command:
	lock.unlock();

	po::options_description ldesc("Command options");
	
	if (command)	
		command->InitParameters(ldesc);
		
	desc.add(ldesc);
	
	po::store(po::parse_command_line(argc - arg_end, argv + arg_end, desc), vm);
	po::notify(vm);

	return true;
}

void CLICommand::ShowCommands(int argc, char **argv, po::options_description *desc, bool autocomplete)
{
	boost::mutex::scoped_lock lock(l_RegistryMutex);

	typedef std::map<std::vector<String>, CLICommand::Ptr>::value_type CLIKeyValue;

	std::vector<String> best_match;
	int arg_begin = 0;
	CLICommand::Ptr command;

	BOOST_FOREACH(const CLIKeyValue& kv, l_Registry) {
		const std::vector<String>& vname = kv.first;

		arg_begin = 0;
		
		for (int i = 0, k = 1; i < vname.size() && k < argc; i++, k++) {
			if (strcmp(argv[k], "--no-stack-rlimit") == 0 || strcmp(argv[k], "--autocomplete") == 0) {
				i--;
				arg_begin++;
				continue;
			}

			if (vname[i] != argv[k])
				break;
			
			if (i >= best_match.size()) {
				best_match.push_back(vname[i]);
			}
			
			if (i == vname.size() - 1) {
				command = kv.second;
				break;
			}
		}
	}

	if (!autocomplete)
		std::cout << "Supported commands: " << std::endl;
	
	BOOST_FOREACH(const CLIKeyValue& kv, l_Registry) {
		const std::vector<String>& vname = kv.first;
 
		if (vname.size() < best_match.size())
			continue;
 
		bool match = true;
 
		for (int i = 0; i < best_match.size(); i++) {
			if (vname[i] != best_match[i]) {
				match = false;
				break;
			}
		}
 
		if (!match)
			continue;

		if (autocomplete) {
			if (best_match.size() < vname.size()) {
				String cname = vname[best_match.size()];
				String pname;
				
				if (arg_begin + best_match.size() + 1 < argc)
					pname = argv[arg_begin + best_match.size() + 1];
				
				if (cname.Find(pname) == 0)
					std::cout << vname[best_match.size()] << " ";
			}
		} else
			std::cout << "  * " << boost::algorithm::join(vname, " ") << " (" << kv.second->GetShortDescription() << ")" << std::endl;
	}
	
	if (command && autocomplete) {
		po::options_description ldesc("Command options");
		
		if (command)	
			command->InitParameters(ldesc);
			
		desc->add(ldesc);

		BOOST_FOREACH(const shared_ptr<po::option_description>& odesc, desc->options()) {
			String cname = "--" + odesc->long_name();
			String pname = argv[argc - 1];
			
			if (cname.Find(pname) == 0)
				std::cout << cname << " ";
		}
	}

	return;
}
