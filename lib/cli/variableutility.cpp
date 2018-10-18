/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include "base/stdiostream.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "remote/jsonrpc.hpp"
#include <fstream>

using namespace icinga;

Value VariableUtility::GetVariable(const String& name)
{
	String varsfile = Configuration::VarsPath;

	std::fstream fp;
	fp.open(varsfile.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	String message;
	StreamReadContext src;
	for (;;) {
		StreamReadStatus srs = NetString::ReadStringFromStream(sfp, &message, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		Dictionary::Ptr variable = JsonDecode(message);

		if (variable->Get("name") == name) {
			return variable->Get("value");
		}
	}

	return Empty;
}

void VariableUtility::PrintVariables(std::ostream& outfp)
{
	String varsfile = Configuration::VarsPath;

	std::fstream fp;
	fp.open(varsfile.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = new StdioStream(&fp, false);
	unsigned long variables_count = 0;

	String message;
	StreamReadContext src;
	for (;;) {
		StreamReadStatus srs = NetString::ReadStringFromStream(sfp, &message, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		Dictionary::Ptr variable = JsonDecode(message);
		outfp << variable->Get("name") << " = " << variable->Get("value") << "\n";
		variables_count++;
	}

	sfp->Close();
	fp.close();

	Log(LogNotice, "cli")
		<< "Parsed " << variables_count << " variables.";
}
