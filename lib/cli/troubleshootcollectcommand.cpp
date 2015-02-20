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
#include "cli/objectlistutility.hpp"
#include "cli/featureutility.hpp"
#include "cli/daemonutility.hpp"
#include "base/netstring.hpp"
#include "base/application.hpp"
#include "base/stdiostream.hpp"
#include "base/json.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"

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

class TroubleshootCollectCommand::InfoLog
{
public:
	InfoLog(const String& path, const bool cons)
	{
		m_Console = cons;
		if (m_Console)
			m_Stream = new std::ostream(std::cout.rdbuf());
		else {
			std::ofstream *ofs = new std::ofstream();
			ofs->open(path.CStr(), std::ios::out | std::ios::trunc);
			m_Stream = ofs;
		}
	}

	void WriteLine(const LogSeverity sev, const String& str)
	{
		if (!m_Console)
			Log(sev, "troubleshoot", str);

		if (sev == LogCritical || sev == LogWarning) {
			*m_Stream << std::string(24, '#') << '\n'
				<< "# " << str << '\n'
				<< std::string(24, '#') << '\n';
		} else
			*m_Stream << str << '\n';
	}

	bool GetStreamHealth(void) const
	{
		return *m_Stream;
	}

private:
	bool m_Console;
	std::ostream *m_Stream;
};

class TroubleshootCollectCommand::InfoLogLine
{
public:
	InfoLogLine(InfoLog& log, LogSeverity sev = LogInformation)
		: m_Log(log), m_Sev(sev) {}

	~InfoLogLine()
	{
		m_Log.WriteLine(m_Sev, m_String.str());
	}

	template <typename T>
	InfoLogLine& operator<<(const T& info)
	{
		m_String << info;
		return *this;
	}

private:
	std::ostringstream m_String;
	InfoLog& m_Log;
	LogSeverity m_Sev;
};


bool TroubleshootCollectCommand::GeneralInfo(InfoLog& log, const boost::program_options::variables_map& vm)
{
	InfoLogLine(log) << '\n' << std::string(14, '=') << " GENERAL INFORMATION " << std::string(14, '=') << '\n';

	//Application::DisplayInfoMessage() but formatted 
	InfoLogLine(log)
		<< "\tApplication version: " << Application::GetVersion() << '\n'
		<< "\tInstallation root: " << Application::GetPrefixDir() << '\n'
		<< "\tSysconf directory: " << Application::GetSysconfDir() << '\n'
		<< "\tRun directory: " << Application::GetRunDir() << '\n'
		<< "\tLocal state directory: " << Application::GetLocalStateDir() << '\n'
		<< "\tPackage data directory: " << Application::GetPkgDataDir() << '\n'
		<< "\tState path: " << Application::GetStatePath() << '\n'
		<< "\tObjects path: " << Application::GetObjectsPath() << '\n'
		<< "\tVars path: " << Application::GetVarsPath() << '\n'
		<< "\tPID path: " << Application::GetPidPath() << '\n'
		<< "\tApplication type: " << Application::GetApplicationType() << '\n';

	return true;
}

bool TroubleshootCollectCommand::FeatureInfo(InfoLog& log, const boost::program_options::variables_map& vm)
{
	TroubleshootCollectCommand::CheckFeatures(log);
	//TODO Check whether active faetures are operational.
	return true;
}

bool TroubleshootCollectCommand::ObjectInfo(InfoLog& log, const boost::program_options::variables_map& vm, Dictionary::Ptr& logs)
{
	InfoLogLine(log) << '\n' << std::string(14, '=') << " OBJECT INFORMATION " << std::string(14, '=') << '\n';

	String objectfile = Application::GetObjectsPath();
	std::set<String> configs;

	if (!Utility::PathExists(objectfile)) {
		InfoLogLine(log, LogCritical) << "Cannot open object file '" << objectfile << "'.\n"
			<< "FAILED: This probably means you have a fault configuration.";
		return false;
	} else
		CheckObjectFile(objectfile, log, vm.count("include-objects"), logs, configs);

	return true;
}

bool TroubleshootCollectCommand::ReportInfo(InfoLog& log, const boost::program_options::variables_map& vm, Dictionary::Ptr& logs)
{
	InfoLogLine(log) << '\n' << std::string(14, '=') << " LOGS AND CRASH REPORTS " << std::string(14, '=') << '\n';
	PrintLoggers(log, logs);
	PrintCrashReports(log);

	return true;
}

bool TroubleshootCollectCommand::ConfigInfo(InfoLog& log, const boost::program_options::variables_map& vm)
{
	InfoLogLine(log) << '\n' << std::string(14, '=') << " CONFIGURATION FILES " << std::string(14, '=') << '\n';

	InfoLogLine(log) << "A collection of important configuration files follows, please make sure to remove any sensitive data such as credentials, internal company names, etc";
	if (!PrintConf(log, Application::GetSysconfDir() + "/icinga2/icinga2.conf")) {
		InfoLogLine(log, LogWarning) << "icinga2.conf not found, therefore skipping validation.\n"
			<< "If you are using an icinga2.conf somewhere but the default path please validate it via 'icinga2 daemon -C -c \"path\to/icinga2.conf\"'\n"
			<< "and provide it with your support request.";
	}

	if (!PrintConf(log, Application::GetSysconfDir() + "/icinga2/zones.conf")) {
		InfoLogLine(log, LogWarning) << "zones.conf not found.\n"
			<< "If you are using a zones.conf somewhere but the default path please provide it with your support request";
	}

	return true;
}


/*Print the last *numLines* of *file* to *os* */
int TroubleshootCollectCommand::Tail(const String& file, int numLines, InfoLog& log)
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
		InfoLogLine(log) << '\t' << ringBuf[k];
	text.close();
	InfoLogLine(log) << "[end: '" << file << "' line: " << lines << ']';
	return numLines;
}

bool TroubleshootCollectCommand::CheckFeatures(InfoLog& log)
{
	Dictionary::Ptr features = new Dictionary;
	std::vector<String> disabled_features;
	std::vector<String> enabled_features;

	if (!FeatureUtility::GetFeatures(disabled_features, true)
		|| !FeatureUtility::GetFeatures(enabled_features, false)) {
		InfoLogLine(log, LogCritical) << "Failed to collect enabled and/or disabled features. Check\n"
			<< FeatureUtility::GetFeaturesAvailablePath() << '\n'
			<< FeatureUtility::GetFeaturesEnabledPath();
		return false;
	}

	BOOST_FOREACH(const String feature, disabled_features)
		features->Set(feature, false);
	BOOST_FOREACH(const String feature, enabled_features)
		features->Set(feature, true);

	InfoLogLine(log) << "Enabled features:\n\t" << boost::algorithm::join(enabled_features, " ") << '\n'
		<< "Disabled features:\n\t" << boost::algorithm::join(disabled_features, " ") << '\n';

	if (!features->Get("checker").ToBool())
		InfoLogLine(log, LogWarning) << "checker is disabled, no checks can be run from this instance";
	if (!features->Get("mainlog").ToBool())
		InfoLogLine(log, LogWarning) << "mainlog is disabled, please activate it and rerun icinga2";
	if (!features->Get("debuglog").ToBool())
		InfoLogLine(log, LogWarning) << "debuglog is disabled, please activate it and rerun icinga2";
	return true;
}

void TroubleshootCollectCommand::GetLatestReport(const String& filename, time_t& bestTimestamp, String& bestFilename)
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

bool TroubleshootCollectCommand::PrintCrashReports(InfoLog& log)
{
	String spath = Application::GetLocalStateDir() + "/log/icinga2/crash/report.*";
	time_t bestTimestamp = 0;
	String bestFilename;

	try {
		Utility::Glob(spath,
					boost::bind(&GetLatestReport, _1, boost::ref(bestTimestamp), boost::ref(bestFilename)), 
					  GlobFile);
	}
#ifdef _WIN32
	catch (win32_error &ex) {
		if (int const * err = boost::get_error_info<errinfo_win32_error>(ex)) {
			if (*err != 3) {//Error code for path does not exist
				InfoLogLine(log, LogWarning) << Application::GetLocalStateDir() << "/log/icinga2/crash/ does not exist";
				return false;
			}
		} 
		InfoLogLine(log, LogWarning) << "Error printing crash reports";
		return false;
	}
#else
	catch (...) {
		InfoLogLine(log, LogWarning) << "Error printing crash reports.\nDoes " 
			<< Application::GetLocalStateDir() << "/log/icinga2/crash/ exist?";
			return false;
	}
#endif /*_WIN32*/

	if (!bestTimestamp)
		InfoLogLine(log) << "No crash logs found in " << Application::GetLocalStateDir().CStr() << "/log/icinga2/crash/";
	else {
		InfoLogLine(log) << "Latest crash report is from " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", Utility::GetTime())
			<< "\nFile: " << bestFilename;
		Tail(bestFilename, 20, log);
	}
	return true;
}

bool TroubleshootCollectCommand::PrintConf(InfoLog& log, const String& path)
{
	std::ifstream text;
	text.open(path.CStr(), std::ifstream::in);
	if (!text.is_open())
		return false;

	std::string line;

	InfoLogLine(log) << "\n[begin: '" << path << "']";
	while (std::getline(text, line)) {
		InfoLogLine(log) << '\t' << line;
	}
	InfoLogLine(log) << "\n[end: '" << path << "']";
	return true;
}

bool TroubleshootCollectCommand::CheckConfig(void)
{
	/* Not loading the icinga library would make config validation fail.
	 * (Depending on the configuration and the speed of your machine.) 
	 */
	Utility::LoadExtensionLibrary("icinga");
	std::vector<std::string> configs;
	configs.push_back(Application::GetSysconfDir() + "/icinga2/icinga2.conf");
	return DaemonUtility::ValidateConfigFiles(configs, Application::GetObjectsPath());
}

void TroubleshootCollectCommand::CheckObjectFile(const String& objectfile, InfoLog& log, const bool print,
							Dictionary::Ptr& logs, std::set<String>& configs)
{
	InfoLogLine(log) << "Checking object file from " << objectfile;

	std::fstream fp;
	fp.open(objectfile.CStr(), std::ios_base::in);

	if (!fp.is_open()) {
		InfoLogLine(log, LogWarning) << "Could not open object file.";
		return;
	}
	
	StdioStream::Ptr sfp = new StdioStream(&fp, false);
	String::SizeType typeL = 0, countTotal = 0;

	String message;
	StreamReadContext src;
	StreamReadStatus srs;
	std::map<String, int> type_count;
	bool first = true;
	while ((srs = NetString::ReadStringFromStream(sfp, &message, src)) != StatusEof) {
		if (srs != StatusNewItem)
			continue;

		bool first = true;
		
		std::stringstream sStream;
		
		ObjectListUtility::PrintObject(sStream, first, message, type_count, "", "");
		
		Dictionary::Ptr object = JsonDecode(message);
		Dictionary::Ptr properties = object->Get("properties");

		String name = object->Get("name");
		String type = object->Get("type");

		//Find longest typename for padding
		typeL = type.GetLength() > typeL ? type.GetLength() : typeL;
		countTotal++;

		Array::Ptr debug_info = object->Get("debug_info");
		if (debug_info) {
			configs.insert(debug_info->Get(0));
		}

		if (Utility::Match(type, "FileLogger")) {
			Dictionary::Ptr debug_hints = object->Get("debug_hints");
			Dictionary::Ptr properties = object->Get("properties");

			ObjectLock olock(properties);
			BOOST_FOREACH(const Dictionary::Pair& kv, properties) {
				if (Utility::Match(kv.first, "path"))
					logs->Set(name, kv.second);
			}
		}
	}

	if (!countTotal) {
		InfoLogLine(log, LogCritical) << "No objects found in objectfile.";
		return;
	}

	//Print objects with count
	InfoLogLine(log) << "Found the " << countTotal << " objects:"
		<< "\tType" << std::string(typeL-4, ' ') << " : Count";

	BOOST_FOREACH(const Dictionary::Pair& kv, type_count) {
		InfoLogLine(log) << '\t' << kv.first << std::string(typeL - kv.first.GetLength(), ' ')
			<< " : " << kv.second;
	}
}

void TroubleshootCollectCommand::PrintLoggers(InfoLog& log, Dictionary::Ptr& logs)
{
	if (!logs->GetLength()) {
		InfoLogLine(log, LogWarning) << "No loggers found, check whether you enabled any logging features";
	} else {
		InfoLogLine(log) << "Getting the last 20 lines of " << logs->GetLength() << " FileLogger objects.";
		ObjectLock ulock(logs);
		BOOST_FOREACH(const Dictionary::Pair& kv, logs)
		{
			InfoLogLine(log) << "\nLogger " << kv.first << " at path: " << kv.second;
			if (!Tail(kv.second, 20, log))
				InfoLogLine(log, LogWarning) << kv.second << " either does not exist or is empty";
		}
	}
}

void TroubleshootCollectCommand::PrintConfig(InfoLog& log, const std::set<String>& configSet, const String::SizeType& countTotal)
{
	InfoLogLine(log) << countTotal << " objects in total, originating from these files:";
	for (std::set<String>::iterator it = configSet.begin();
		 it != configSet.end(); it++)
		 InfoLogLine(log) << '\t' << *it;
}

void TroubleshootCollectCommand::InitParameters(boost::program_options::options_description& visibleDesc,
												boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("console,c", "print to console instead of file")
		("output,o", boost::program_options::value<std::string>(), "path to output file")
		("include-objects", "Print the whole objectfile (like `object list`)")
		;
}

int TroubleshootCollectCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String path;
	InfoLog *log;
	Logger::SetConsoleLogSeverity(LogWarning);

	if (vm.count("console")) {
		log = new InfoLog("", true);
	} else {
		if (vm.count("output"))
			path = vm["output"].as<std::string>();
		else {
#ifdef _WIN32 //Dislikes ':' in filenames
			path = Application::GetLocalStateDir() + "/log/icinga2/troubleshooting-"
				+ Utility::FormatDateTime("%Y-%m-%d_%H-%M-%S", Utility::GetTime()) + ".log";
#else
			path = Application::GetLocalStateDir() + "/log/icinga2/troubleshooting-" 
				+ Utility::FormatDateTime("%Y-%m-%d_%H:%M:%S", Utility::GetTime()) + ".log";
#endif /*_WIN32*/
		}
		log = new InfoLog(path, false);
		if (!log->GetStreamHealth()) {
			Log(LogCritical, "troubleshoot", "Failed to open file to write: " + path);
			return 3;
		}
	}	
	String appName = Utility::BaseName(Application::GetArgV()[0]);
	double goTime = Utility::GetTime();

	InfoLogLine(*log) << appName << " -- Troubleshooting help:\n"
		<< "Should you run into problems with Icinga please add this file to your help request\n"
		<< "Began procedure at timestamp " << Convert::ToString(goTime) << '\n';

	if (appName.GetLength() > 3 && appName.SubStr(0, 3) == "lt-")
		appName = appName.SubStr(3, appName.GetLength() - 3);

	Dictionary::Ptr logs = new Dictionary;

	if (!GeneralInfo(*log, vm)
		|| !FeatureInfo(*log, vm)
		|| !ObjectInfo(*log, vm, logs)
		|| !ReportInfo(*log, vm, logs)
		|| !ConfigInfo(*log, vm)) {
		InfoLogLine(*log, LogCritical) << "Could not recover from critical failure, exiting.";
		delete log;
		return 3;
	}
	
	double endTime = Utility::GetTime();
	InfoLogLine(*log) << "\nFinished collection at timestamp " << Convert::ToString(endTime)
		<< "\nTook " << Convert::ToString(endTime - goTime) << " seconds\n";
	if (!vm.count("console")) {
		std::cout << "\nFinished collection. See '" << path << "'\n";
	}

	delete log;
	return 0;
}
