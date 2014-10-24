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

#include "cli/variablelistcommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/dynamicobject.hpp"
#include "base/dynamictype.hpp"
#include "base/serializer.hpp"
#include "base/netstring.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("variable/list", VariableListCommand);

String VariableListCommand::GetDescription(void) const
{
	return "Lists all Icinga 2 variables.";
}

String VariableListCommand::GetShortDescription(void) const
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
	String varsfile = Application::GetVarsPath();

	if (!Utility::PathExists(varsfile)) {
		Log(LogCritical, "cli")
		    << "Cannot open variables file '" << varsfile << "'.";
		Log(LogCritical, "cli", "Run 'icinga2 daemon -C' to validate config and generate the cache file.");
		return 1;
	}

	std::fstream fp;
	fp.open(varsfile.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = make_shared<StdioStream>(&fp, false);
	unsigned long variables_count = 0;

	String message;

	while (NetString::ReadStringFromStream(sfp, &message)) {
		PrintVariable(std::cout, message);
		variables_count++;
	}

	sfp->Close();
	fp.close();

	Log(LogNotice, "cli")
	    << "Parsed " << variables_count << " variables.";

	return 0;
}

void VariableListCommand::PrintVariable(std::ostream& fp, const String& message)
{
	Dictionary::Ptr variable = JsonDeserialize(message);

	std::cout << variable->Get("name") << " = " << variable->Get("value") << "\n";
}
