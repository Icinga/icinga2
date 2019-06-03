/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/objectlistcommand.hpp"
#include "cli/objectlistutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/configobject.hpp"
#include "base/configtype.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("object/list", ObjectListCommand);

String ObjectListCommand::GetDescription() const
{
	return "Lists all Icinga 2 objects.";
}

String ObjectListCommand::GetShortDescription() const
{
	return "lists all objects";
}

void ObjectListCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("count,c", "display object counts by types")
		("name,n", po::value<std::string>(), "filter by name matches")
		("type,t", po::value<std::string>(), "filter by type matches");
}

/**
 * The entry point for the "object list" CLI command.
 *
 * @returns An exit status.
 */
int ObjectListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String objectfile = Configuration::ObjectsPath;

	if (!Utility::PathExists(objectfile)) {
		Log(LogCritical, "cli")
			<< "Cannot open objects file '" << Configuration::ObjectsPath << "'.";
		Log(LogCritical, "cli", "Run 'icinga2 daemon -C' to validate config and generate the cache file.");
		return 1;
	}

	std::fstream fp;
	fp.open(objectfile.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = new StdioStream(&fp, false);
	unsigned long objects_count = 0;
	std::map<String, int> type_count;

	String name_filter, type_filter;

	if (vm.count("name"))
		name_filter = vm["name"].as<std::string>();
	if (vm.count("type"))
		type_filter = vm["type"].as<std::string>();

	bool first = true;

	String message;
	StreamReadContext src;
	for (;;) {
		StreamReadStatus srs = NetString::ReadStringFromStream(sfp, &message, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		ObjectListUtility::PrintObject(std::cout, first, message, type_count, name_filter, type_filter);
		objects_count++;
	}

	sfp->Close();
	fp.close();

	if (vm.count("count")) {
		if (!first)
			std::cout << "\n";

		PrintTypeCounts(std::cout, type_count);
		std::cout << "\n";
	}

	Log(LogNotice, "cli")
		<< "Parsed " << objects_count << " objects.";

	return 0;
}

void ObjectListCommand::PrintTypeCounts(std::ostream& fp, const std::map<String, int>& type_count)
{
	typedef std::map<String, int>::value_type TypeCount;

	for (const TypeCount& kv : type_count) {
		fp << "Found " << kv.second << " " << kv.first << " object";

		if (kv.second != 1)
			fp << "s";

		fp << ".\n";
	}
}
