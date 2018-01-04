/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "cli/clicommand.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include "base/type.hpp"
#include "base/serializer.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

std::vector<String> icinga::GetBashCompletionSuggestions(const String& type, const String& word)
{
	std::vector<String> result;

#ifndef _WIN32
	String bashArg = "compgen -A " + Utility::EscapeShellArg(type) + " " + Utility::EscapeShellArg(word);
	String cmd = "bash -c " + Utility::EscapeShellArg(bashArg);

	FILE *fp = popen(cmd.CStr(), "r");

	char line[4096];
	while (fgets(line, sizeof(line), fp)) {
		String wline = line;
		boost::algorithm::trim_right_if(wline, boost::is_any_of("\r\n"));
		result.push_back(wline);
	}

	pclose(fp);

	/* Append a slash if there's only one suggestion and it's a directory */
	if ((type == "file" || type == "directory") && result.size() == 1) {
		String path = result[0];

		struct stat statbuf;
		if (lstat(path.CStr(), &statbuf) >= 0) {
			if (S_ISDIR(statbuf.st_mode)) {
				result.clear(),
				result.push_back(path + "/");
			}
		}
	}
#endif /* _WIN32 */

	return result;
}

std::vector<String> icinga::GetFieldCompletionSuggestions(const Type::Ptr& type, const String& word)
{
	std::vector<String> result;

	for (int i = 0; i < type->GetFieldCount(); i++) {
		Field field = type->GetFieldInfo(i);

		if (field.Attributes & FANoUserView)
			continue;

		if (strcmp(field.TypeName, "int") != 0 && strcmp(field.TypeName, "double") != 0
			&& strcmp(field.TypeName, "bool") != 0 && strcmp(field.TypeName, "String") != 0)
			continue;

		String fname = field.Name;

		String suggestion = fname + "=";

		if (suggestion.Find(word) == 0)
			result.push_back(suggestion);
	}

	return result;
}

int CLICommand::GetMinArguments() const
{
	return 0;
}

int CLICommand::GetMaxArguments() const
{
	return GetMinArguments();
}

bool CLICommand::IsHidden() const
{
	return false;
}

bool CLICommand::IsDeprecated() const
{
	return false;
}

boost::mutex& CLICommand::GetRegistryMutex()
{
	static boost::mutex mtx;
	return mtx;
}

std::map<std::vector<String>, CLICommand::Ptr>& CLICommand::GetRegistry()
{
	static std::map<std::vector<String>, CLICommand::Ptr> registry;
	return registry;
}

CLICommand::Ptr CLICommand::GetByName(const std::vector<String>& name)
{
	boost::mutex::scoped_lock lock(GetRegistryMutex());

	auto it = GetRegistry().find(name);

	if (it == GetRegistry().end())
		return nullptr;

	return it->second;
}

void CLICommand::Register(const std::vector<String>& name, const CLICommand::Ptr& function)
{
	boost::mutex::scoped_lock lock(GetRegistryMutex());
	GetRegistry()[name] = function;
}

void CLICommand::Unregister(const std::vector<String>& name)
{
	boost::mutex::scoped_lock lock(GetRegistryMutex());
	GetRegistry().erase(name);
}

std::vector<String> CLICommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	return std::vector<String>();
}

std::vector<String> CLICommand::GetPositionalSuggestions(const String& word) const
{
	return std::vector<String>();
}

void CLICommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{ }

ImpersonationLevel CLICommand::GetImpersonationLevel() const
{
	return ImpersonateIcinga;
}

bool CLICommand::ParseCommand(int argc, char **argv, po::options_description& visibleDesc,
	po::options_description& hiddenDesc,
	po::positional_options_description& positionalDesc,
	po::variables_map& vm, String& cmdname, CLICommand::Ptr& command, bool autocomplete)
{
	boost::mutex::scoped_lock lock(GetRegistryMutex());

	typedef std::map<std::vector<String>, CLICommand::Ptr>::value_type CLIKeyValue;

	std::vector<String> best_match;
	int arg_end = 0;
	bool tried_command = false;

	for (const CLIKeyValue& kv : GetRegistry()) {
		const std::vector<String>& vname = kv.first;

		std::vector<String>::size_type i;
		int k;
		for (i = 0, k = 1; i < vname.size() && k < argc; i++, k++) {
			if (strncmp(argv[k], "--", 2) == 0) {
				i--;
				continue;
			}

			tried_command = true;

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

	if (command) {
		po::options_description vdesc("Command options");
		command->InitParameters(vdesc, hiddenDesc);
		visibleDesc.add(vdesc);
	}

	if (autocomplete || (tried_command && !command))
		return true;

	po::options_description adesc;
	adesc.add(visibleDesc);
	adesc.add(hiddenDesc);

	if (command && command->IsDeprecated()) {
		std::cerr << ConsoleColorTag(Console_ForegroundRed | Console_Bold)
			<< "Warning: CLI command '" << cmdname << "' is DEPRECATED! Please read the Changelog."
			<< ConsoleColorTag(Console_Normal) << std::endl << std::endl;
	}

	po::store(po::command_line_parser(argc - arg_end, argv + arg_end).options(adesc).positional(positionalDesc).run(), vm);
	po::notify(vm);

	return true;
}

void CLICommand::ShowCommands(int argc, char **argv, po::options_description *visibleDesc,
	po::options_description *hiddenDesc,
	ArgumentCompletionCallback globalArgCompletionCallback,
	bool autocomplete, int autoindex)
{
	boost::mutex::scoped_lock lock(GetRegistryMutex());

	typedef std::map<std::vector<String>, CLICommand::Ptr>::value_type CLIKeyValue;

	std::vector<String> best_match;
	int arg_begin = 0;
	CLICommand::Ptr command;

	for (const CLIKeyValue& kv : GetRegistry()) {
		const std::vector<String>& vname = kv.first;

		arg_begin = 0;

		std::vector<String>::size_type i;
		int k;
		for (i = 0, k = 1; i < vname.size() && k < argc; i++, k++) {
			if (strcmp(argv[k], "--no-stack-rlimit") == 0 || strcmp(argv[k], "--autocomplete") == 0 || strcmp(argv[k], "--scm") == 0) {
				i--;
				arg_begin++;
				continue;
			}

			if (autocomplete && static_cast<int>(i) >= autoindex - 1)
				break;

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

	String aword;

	if (autocomplete) {
		if (autoindex < argc)
			aword = argv[autoindex];

		if (autoindex - 1 > static_cast<int>(best_match.size()) && !command)
			return;
	} else
		std::cout << "Supported commands: " << std::endl;

	for (const CLIKeyValue& kv : GetRegistry()) {
		const std::vector<String>& vname = kv.first;

		if (vname.size() < best_match.size() || kv.second->IsHidden())
			continue;

		bool match = true;

		for (std::vector<String>::size_type i = 0; i < best_match.size(); i++) {
			if (vname[i] != best_match[i]) {
				match = false;
				break;
			}
		}

		if (!match)
			continue;

		if (autocomplete) {
			String cname;

			if (autoindex - 1 < static_cast<int>(vname.size())) {
				cname = vname[autoindex - 1];

				if (cname.Find(aword) == 0)
					std::cout << cname << "\n";
			}
		} else {
			std::cout << "  * " << boost::algorithm::join(vname, " ")
				<< " (" << kv.second->GetShortDescription() << ")"
				<< (kv.second->IsDeprecated() ? " (DEPRECATED)" : "") << std::endl;
		}
	}

	if (!autocomplete)
		std::cout << std::endl;

	if (command && autocomplete) {
		String aname, prefix, pword;
		const po::option_description *odesc;

		if (autoindex - 2 >= 0 && strcmp(argv[autoindex - 1], "=") == 0 && strstr(argv[autoindex - 2], "--") == argv[autoindex - 2]) {
			aname = argv[autoindex - 2] + 2;
			pword = aword;
		} else if (autoindex - 1 >= 0 && argv[autoindex - 1][0] == '-' && argv[autoindex - 1][1] == '-') {
			aname = argv[autoindex - 1] + 2;
			pword = aword;

			if (pword == "=")
				pword = "";
		} else if (autoindex - 1 >= 0 && argv[autoindex - 1][0] == '-' && argv[autoindex - 1][1] != '-') {
			aname = argv[autoindex - 1];
			pword = aword;

			if (pword == "=")
				pword = "";
		} else if (aword.GetLength() > 1 && aword[0] == '-' && aword[1] != '-') {
			aname = aword.SubStr(0, 2);
			prefix = aname;
			pword = aword.SubStr(2);
		} else {
			goto complete_option;
		}

		odesc = visibleDesc->find_nothrow(aname, false);

		if (!odesc)
			return;

		if (odesc->semantic()->min_tokens() == 0)
			goto complete_option;

		for (const String& suggestion : globalArgCompletionCallback(odesc->long_name(), pword)) {
			std::cout << prefix << suggestion << "\n";
		}

		for (const String& suggestion : command->GetArgumentSuggestions(odesc->long_name(), pword)) {
			std::cout << prefix << suggestion << "\n";
		}

		return;

complete_option:
		for (const boost::shared_ptr<po::option_description>& odesc : visibleDesc->options()) {
			String cname = "--" + odesc->long_name();

			if (cname.Find(aword) == 0)
				std::cout << cname << "\n";
		}

		for (const String& suggestion : command->GetPositionalSuggestions(aword)) {
			std::cout << suggestion << "\n";
		}
	}

	return;
}
