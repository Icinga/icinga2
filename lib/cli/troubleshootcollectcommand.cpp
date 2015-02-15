/*****************************************************************************
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

#include "cli/troubleshootcollectcommand.hpp"
#include "cli/featureutility.hpp"
#include "cli/daemonutility.hpp"
#include "base/netstring.hpp"
#include "base/application.hpp"
#include "base/stdiostream.hpp"
#include "base/json.hpp"
#include "base/objectlock.hpp"

#include "config/configitembuilder.hpp"

#include <boost/circular_buffer.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <iostream>
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("troubleshoot/collect", TroubleshootCollectCommand);

String TroubleshootCollectCommand::GetDescription(void) const
{
	return "Collect logs and other relevant information for troubleshooting purposes.";
}

String TroubleshootCollectCommand::GetShortDescription(void) const
{
	return "Collect information for troubleshooting";
}

static void GetLatestReport(const String& filename, time_t& bestTimestamp, String& bestFilename)
{
#ifdef _WIN32
	struct _stat buf;
	if (_stat(filename.CStr(), &buf))
		return;
#else
	struct stat buf;
	if (stat(filename.CStr(), &buf))
		return;
#endif /*_WIN32*/
	if (buf.st_mtime > bestTimestamp) {
		bestTimestamp = buf.st_mtime;
		bestFilename = filename;
	}
}

/*Print the latest crash report to *os* */
static void PrintCrashReports(std::ostream& os)
{
	String spath = Application::GetLocalStateDir() + "/log/icinga2/crash/report.*";
	time_t bestTimestamp = 0;
	String bestFilename;

	try {
		Utility::Glob(spath,
					  boost::bind(&GetLatestReport, _1, boost::ref(bestTimestamp), boost::ref(bestFilename)), GlobFile);
	}
		
#ifdef _WIN32
	catch (win32_error &ex) {
		if (int const * err = boost::get_error_info<errinfo_win32_error>(ex)) {
			if (*err != 3) //Error code for path does not exist
				throw ex;
			os << Application::GetLocalStateDir() + "/log/icinga2/crash/ does not exist\n";
		} else {
			throw ex;
		}
	}
#else
	catch (...) {
		throw;
	}
#endif /*_WIN32*/

		
	if (!bestTimestamp)
		os << "\nNo crash logs found in " << Application::GetLocalStateDir().CStr() << "/log/icinga2/crash/\n";
	else {
		const std::tm tm = Utility::LocalTime(bestTimestamp);
				char *tmBuf = new char[200]; //Should always be enough
				const char *fmt = "%Y-%m-%d %H:%M:%S" ;
				if (!strftime(tmBuf, 199, fmt, &tm))
					return;
		os << "\nLatest crash report is from " << tmBuf
			<< "\nFile: " << bestFilename << '\n';
		TroubleshootCollectCommand::tail(bestFilename, 20, os);
	}
}

/*Print the last *numLines* of *file* to *os* */
int TroubleshootCollectCommand::tail(const String& file, int numLines, std::ostream& os)
{
	boost::circular_buffer<std::string> ringBuf(numLines);
	std::ifstream text;
	text.open(file.CStr(), std::ifstream::in);
	if (!text.good())
		return 0;

	std::string line;
	int lines = 0;

	while (std::getline(text, line)) {
		ringBuf.push_back(line);
		lines++;
	}

	if (lines < numLines)
		numLines = lines;

	for (int k = 0; k < numLines; k++)
		os << '\t' << ringBuf[k] << '\n';;

	text.close();
	return numLines;
}

static bool PrintIcingaConf(std::ostream& os)
{
	String path = Application::GetSysconfDir() + "/icinga2/icinga2.conf";

	std::ifstream text;
	text.open(path.CStr(), std::ifstream::in);
	if (!text.is_open()) {
		Log(LogCritical, "troubleshooting", "Could not find icinga2.conf at its default location (" + path + ")");
		os << "! Could not open " << path
			<< "\n!\tIf you use a custom icinga2.conf provide it after validating it via `icinga2 daemon -C`"
			<< "\n!\tIf you do not have a icinga2.conf you just found your problem.\n";
		return false;
	}
	std::string line;

	os << "\nFound main Icinga2 configuration file at " << path << '\n';
	while (std::getline(text, line)) {
		os << '\t' << line << '\n';
	}
	return true;
}

static bool PrintZonesConf(std::ostream& os)
{
	String path = Application::GetSysconfDir() + "/icinga2/zones.conf";

	std::ifstream text;
	text.open(path.CStr(), std::ifstream::in);
	if (!text.is_open()) {
		Log(LogWarning, "troubleshooting", "Could not find zones.conf at its default location (" + path + ")");
		os << "!Could not open " << path
			<< "\n!\tThis could be the root of your problems, if you trying to use multiple Icinga2 instances.\n";
		return false;
	}
	std::string line;

	os << "\nFound zones configuration file at " << path << '\n';
	while (std::getline(text, line)) {
		os << '\t' << line << '\n';
	}
	return true;
}

static void ValidateConfig(std::ostream& os)
{
	/* Not loading the icinga library would make config validation fail.
	   (Depending on the configuration and core count of your machine.) */
	Logger::DisableConsoleLog();
	Utility::LoadExtensionLibrary("icinga");
	std::vector<std::string> configs;
	configs.push_back(Application::GetSysconfDir() + "/icinga2/icinga2.conf");

	if (DaemonUtility::ValidateConfigFiles(configs, Application::GetObjectsPath()))
		os << "Config validation successful\n";
	else
		os << "! Config validation failed\n"
		<< "Run `icinga2 daemon --validate` to recieve additional information\n";
}

static void CheckFeatures(std::ostream& os)
{
	Dictionary::Ptr features = new Dictionary;
	std::vector<String> disabled_features;
	std::vector<String> enabled_features;

	if (!FeatureUtility::GetFeatures(disabled_features, true)
		|| !FeatureUtility::GetFeatures(enabled_features, false)) {
		Log(LogWarning, "troubleshoot", "Could not collect features");
		os << "! Failed to collect enabled and/or disabled features. Check\n"
			<< FeatureUtility::GetFeaturesAvailablePath() << '\n'
			<< FeatureUtility::GetFeaturesEnabledPath() << '\n';
		return;
	}

	BOOST_FOREACH(const String feature, disabled_features)
		features->Set(feature, false);
	BOOST_FOREACH(const String feature, enabled_features)
		features->Set(feature, true);

	os  << "Icinga2 feature list\n"
		<< "Enabled features:\n\t" << boost::algorithm::join(enabled_features, " ") << '\n'
		<< "Disabled features:\n\t" << boost::algorithm::join(disabled_features, " ") << '\n';

	if (!features->Get("mainlog").ToBool())
		os << "! mainlog is disabled, please activate it and rerun icinga2\n";
	if (!features->Get("debuglog").ToBool())
		os << "! debuglog is disabled, please activate it and rerun icinga2\n";
}

static void CheckObjectFile(const String& objectfile, std::ostream& os)
{
	os << "Checking object file from " << objectfile << '\n';

	std::fstream fp;
	std::set<String> configSet;
	Dictionary::Ptr typeCount = new Dictionary();
	Dictionary::Ptr logPath = new Dictionary();
	fp.open(objectfile.CStr(), std::ios_base::in);

	if (!fp.is_open()) {
		Log(LogWarning, "troubleshoot", "Could not open objectfile");
		os << "! Could not open object file.\n";
		return;
	}

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	int typeL = 0, countTotal = 0;

	String message;
	StreamReadContext src;
	for (;;) {
		StreamReadStatus srs = NetString::ReadStringFromStream(sfp, &message, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		Dictionary::Ptr object = JsonDecode(message);
		Dictionary::Ptr properties = object->Get("properties");

		String name = object->Get("name");
		String type = object->Get("type");

		//Find longest typename for padding
		typeL = type.GetLength() > typeL ? type.GetLength() : typeL;
		countTotal++;

		if (!typeCount->Contains(type))
			typeCount->Set(type, 1);
		else
			typeCount->Set(type, typeCount->Get(type)+1);

		Array::Ptr debug_info = object->Get("debug_info");
		if (debug_info) {
			configSet.insert(debug_info->Get(0));
		}

		if (Utility::Match(type, "FileLogger")) {
			Dictionary::Ptr debug_hints = object->Get("debug_hints");
			Dictionary::Ptr properties = object->Get("properties");

			ObjectLock olock(properties);
			BOOST_FOREACH(const Dictionary::Pair& kv, properties) {
				if (Utility::Match(kv.first, "path"))
					logPath->Set(name, kv.second);
			}
		}
	}

	if (!countTotal) {
		os << "! No objects found in objectfile.\n";
		return;
	}

	//Print objects with count
	os << "Found the following objects:\n"
		<< "\tType" << std::string(typeL-4, ' ') << " : Count\n";
	ObjectLock olock(typeCount);
	BOOST_FOREACH(const Dictionary::Pair& kv, typeCount) {
		os << '\t' << kv.first << std::string(typeL - kv.first.GetLength(), ' ') 
			<< " : " << kv.second << '\n';
	}

	//Print location of .config files
	os << '\n' << countTotal << " objects in total, originating from these files:\n";
	for (std::set<String>::iterator it = configSet.begin();
		 it != configSet.end(); it++)
		 os << '\t' << *it << '\n';

	//Print tail of file loggers
	if (!logPath->GetLength()) {
		os << "! No loggers found, check whether you enabled any logging features\n";
	} else {
		os << "\nGetting the last 20 lines of the " << logPath->GetLength() << " found FileLogger objects.\n";
		ObjectLock ulock(logPath);
		BOOST_FOREACH(const Dictionary::Pair& kv, logPath)
		{
			os << "\nLogger " << kv.first << " at path: " << kv.second << "\n";
			if (!TroubleshootCollectCommand::tail(kv.second, 20, os))
				os << "\t" << kv.second << " either does not exist or is empty\n";
		}
	}
}

void TroubleshootCollectCommand::InitParameters(boost::program_options::options_description& visibleDesc,
												boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("console,c", "print to console instead of file")
		("output-file", boost::program_options::value<std::string>(), "path to output file")
		;
}

int TroubleshootCollectCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	std::ofstream os;
	String path;
	if (vm.count("console")) {
		Logger::DisableConsoleLog();
		os.copyfmt(std::cout);
		os.clear(std::cout.rdstate());
		os.basic_ios<char>::rdbuf(std::cout.rdbuf());
	} else {
		if (vm.count("output-file"))
			path = vm["output-file"].as<std::string>();
		else
			path = Application::GetLocalStateDir() +"/log/icinga2/troubleshooting.log";
		os.open(path.CStr(), std::ios::out | std::ios::trunc);
		if (!os.is_open()) {
			Log(LogCritical, "troubleshoot", "Failed to open file to write: " + path);
			return 3;
		}
	}
	
	String appName = Utility::BaseName(Application::GetArgV()[0]);

	os << appName << " -- Troubleshooting help:" << std::endl
		<< "Should you run into problems with Icinga please add this file to your help request\n\n";

	if (appName.GetLength() > 3 && appName.SubStr(0, 3) == "lt-")
		appName = appName.SubStr(3, appName.GetLength() - 3);

	//Application::DisplayInfoMessage() but formatted 
	os  << "\tApplication version: " << Application::GetVersion() << "\n"
		<< "\tInstallation root: " << Application::GetPrefixDir() << "\n"
		<< "\tSysconf directory: " << Application::GetSysconfDir() << "\n"
		<< "\tRun directory: " << Application::GetRunDir() << "\n"
		<< "\tLocal state directory: " << Application::GetLocalStateDir() << "\n"
		<< "\tPackage data directory: " << Application::GetPkgDataDir() << "\n"
		<< "\tState path: " << Application::GetStatePath() << "\n"
		<< "\tObjects path: " << Application::GetObjectsPath() << "\n"
		<< "\tVars path: " << Application::GetVarsPath() << "\n"
		<< "\tPID path: " << Application::GetPidPath() << "\n"
		<< "\tApplication type: " << Application::GetApplicationType() << "\n";

	os << '\n';
	CheckFeatures(os);
	os << '\n';

	String objectfile = Application::GetObjectsPath();

	if (!Utility::PathExists(objectfile)) {
		Log(LogWarning, "troubleshoot", "Failed to open objectfile");
		os << "! Cannot open object file '" << objectfile << "'."
			<< "! Run 'icinga2 daemon -C' to validate config and generate the cache file.\n";
	} else
		CheckObjectFile(objectfile, os);

	os << "\nA collection of important configuration files follows, please make sure to censor your sensible data\n";
	if (PrintIcingaConf(os)) {
		ValidateConfig(os);
	} else {
		Log(LogWarning, "troubleshoot", "Failed to open icinga2.conf");
		os << "! icinga2.conf not found, therefore skipping validation.\n";
	}
	os << '\n';
	PrintZonesConf(os);
	os << '\n';

	std::cout << "Finished collection";
	if (!vm.count("console")) {
		os.close();
		std::cout << ", see " << path;
	}
	std::cout << std::endl;

	return 0;
}

