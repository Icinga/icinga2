// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
