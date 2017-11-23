/*****************************************************************************
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

#include "base/application.hpp"
#include "base/console.hpp"
#include "base/convert.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/objectlock.hpp"
#include "base/stdiostream.hpp"
#include "cli/daemonutility.hpp"
#include "cli/featureutility.hpp"
#include "cli/objectlistutility.hpp"
#include "cli/troubleshootcommand.hpp"
#include "cli/variableutility.hpp"
#include "config/configitembuilder.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("troubleshoot", TroubleshootCommand);

String TroubleshootCommand::GetDescription(void) const
{
	return "Collect logs and other relevant information for troubleshooting purposes.";
}

String TroubleshootCommand::GetShortDescription(void) const
{
	return "collect information for troubleshooting";
}

class TroubleshootCommand::InfoLog
{
public:
	InfoLog(const String& path, const bool cons)
	{
		m_Console = cons;
		m_ConsoleType = Console_Dumb;
		if (m_Console) {
			m_Stream = new std::ostream(std::cout.rdbuf());
#ifndef _WIN32
			m_ConsoleType = Console_VT100;
#else /*_WIN32*/
			m_ConsoleType = Console_Windows;
#endif /*_WIN32*/
		}
		else {
			std::ofstream *ofs = new std::ofstream();
			ofs->open(path.CStr(), std::ios::out | std::ios::trunc);
			m_Stream = ofs;
		}
	}

	~InfoLog(void)
	{
		delete m_Stream;
	}

	void WriteLine(const LogSeverity sev, const int color, const String& str)
	{
		if (!m_Console)
			Log(sev, "troubleshoot", str);

		if (sev == LogWarning) {
			*m_Stream
			    << '\n' << ConsoleColorTag(Console_ForegroundYellow, m_ConsoleType) << std::string(24, '#') << '\n'
			    << ConsoleColorTag(Console_Normal, m_ConsoleType) << str
			    << ConsoleColorTag(Console_ForegroundYellow, m_ConsoleType) << std::string(24, '#') << "\n\n"
			    << ConsoleColorTag(Console_Normal, m_ConsoleType);
		} else if (sev == LogCritical) {
			*m_Stream
			    << '\n' << ConsoleColorTag(Console_ForegroundRed, m_ConsoleType) << std::string(24, '#') << '\n'
			    << ConsoleColorTag(Console_Normal, m_ConsoleType) << str
			    << ConsoleColorTag(Console_ForegroundRed, m_ConsoleType) << std::string(24, '#') << "\n\n"
			    << ConsoleColorTag(Console_Normal, m_ConsoleType);
		} else
			*m_Stream
			    << ConsoleColorTag(color, m_ConsoleType) << str
			    << ConsoleColorTag(Console_Normal, m_ConsoleType);
	}

	bool GetStreamHealth(void) const
	{
		return m_Stream->good();
	}

private:
	bool m_Console;
	ConsoleType m_ConsoleType;
	std::ostream *m_Stream;
};

class TroubleshootCommand::InfoLogLine
{
public:
	InfoLogLine(InfoLog& log, int col = Console_Normal, LogSeverity sev = LogInformation)
		: m_Log(log), m_Color(col), m_Sev(sev) {}

	~InfoLogLine(void)
	{
		m_Log.WriteLine(m_Sev, m_Color, m_String.str());
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
	int m_Color;
	LogSeverity m_Sev;
};


bool TroubleshootCommand::GeneralInfo(InfoLog& log, const boost::program_options::variables_map& vm)
{
	InfoLogLine(log, Console_ForegroundBlue)
	    << std::string(14, '=') << " GENERAL INFORMATION " << std::string(14, '=') << "\n\n";

	//Application::DisplayInfoMessage() but formatted
	InfoLogLine(log)
	    << "\tApplication version: " << Application::GetAppVersion() << '\n'
	    << "\tInstallation root: " << Application::GetPrefixDir() << '\n'
	    << "\tSysconf directory: " << Application::GetSysconfDir() << '\n'
	    << "\tRun directory: " << Application::GetRunDir() << '\n'
	    << "\tLocal state directory: " << Application::GetLocalStateDir() << '\n'
	    << "\tPackage data directory: " << Application::GetPkgDataDir() << '\n'
	    << "\tState path: " << Application::GetStatePath() << '\n'
	    << "\tObjects path: " << Application::GetObjectsPath() << '\n'
	    << "\tVars path: " << Application::GetVarsPath() << '\n'
	    << "\tPID path: " << Application::GetPidPath() << '\n';

	InfoLogLine(log)
	    << '\n';

	return true;
}

bool TroubleshootCommand::FeatureInfo(InfoLog& log, const boost::program_options::variables_map& vm)
{
	TroubleshootCommand::CheckFeatures(log);
	//TODO Check whether active features are operational.
	return true;
}

bool TroubleshootCommand::ObjectInfo(InfoLog& log, const boost::program_options::variables_map& vm, Dictionary::Ptr& logs, const String& path)
{
	InfoLogLine(log, Console_ForegroundBlue)
	    << std::string(14, '=') << " OBJECT INFORMATION " << std::string(14, '=') << "\n\n";

	String objectfile = Application::GetObjectsPath();
	std::set<String> configs;

	if (!Utility::PathExists(objectfile)) {
		InfoLogLine(log, 0, LogCritical)
		    << "Cannot open object file '" << objectfile << "'.\n"
		    << "FAILED: This probably means you have a fault configuration.\n";
		return false;
	} else {
		InfoLog *OFile = NULL;
		bool OConsole = false;
		if (vm.count("include-objects")) {
			if (vm.count("console"))
				OConsole = true;
			else {
				OFile = new InfoLog(path+"-objects", false);
				if (!OFile->GetStreamHealth()) {
					InfoLogLine(log, 0, LogWarning)
					    << "Failed to open Object-write-stream, not printing objects\n\n";
					delete OFile;
					OFile = NULL;
				} else
					InfoLogLine(log)
				        << "Printing all objects to " << path+"-objects\n";
			}
		}
		CheckObjectFile(objectfile, log, OFile, OConsole, logs, configs);
		delete OFile;
	}

	if (vm.count("include-vars")) {
		if (vm.count("console")) {
			InfoLogLine(log, Console_ForegroundBlue)
			    << "\n[begin: varsfile]\n";
			if (!PrintVarsFile(path, true))
				InfoLogLine(log, 0, LogWarning)
				    << "Failed to print vars file\n";
			InfoLogLine(log, Console_ForegroundBlue)
			    << "[end: varsfile]\n";
		} else {
			if (PrintVarsFile(path, false))
				InfoLogLine(log)
				    << "Successfully printed all variables to " << path+"-vars\n";
			else
				InfoLogLine(log, 0, LogWarning)
				    << "Failed to print vars to " << path+"-vars\n";
		}
	}

	InfoLogLine(log)
	    << '\n';

	return true;
}

bool TroubleshootCommand::ReportInfo(InfoLog& log, const boost::program_options::variables_map& vm, Dictionary::Ptr& logs)
{
	InfoLogLine(log, Console_ForegroundBlue)
	    << std::string(14, '=') << " LOGS AND CRASH REPORTS " << std::string(14, '=') << "\n\n";
	PrintLoggers(log, logs);
	PrintCrashReports(log);

	InfoLogLine(log)
	    << '\n';

	return true;
}

bool TroubleshootCommand::ConfigInfo(InfoLog& log, const boost::program_options::variables_map& vm)
{
	InfoLogLine(log, Console_ForegroundBlue)
	    << std::string(14, '=') << " CONFIGURATION FILES " << std::string(14, '=') << "\n\n";

	InfoLogLine(log)
	    << "A collection of important configuration files follows, please make sure to remove any sensitive data such as credentials, internal company names, etc\n";

	if (!PrintFile(log, Application::GetSysconfDir() + "/icinga2/icinga2.conf")) {
		InfoLogLine(log, 0, LogWarning)
		    << "icinga2.conf not found, therefore skipping validation.\n"
		    << "If you are using an icinga2.conf somewhere but the default path please validate it via 'icinga2 daemon -C -c \"path\to/icinga2.conf\"'\n"
		    << "and provide it with your support request.\n";
	}

	if (!PrintFile(log, Application::GetSysconfDir() + "/icinga2/zones.conf")) {
		InfoLogLine(log, 0, LogWarning)
		    << "zones.conf not found.\n"
		    << "If you are using a zones.conf somewhere but the default path please provide it with your support request\n";
	}

	InfoLogLine(log)
	   << '\n';

	return true;
}

/*Print the last *numLines* of *file* to *os* */
int TroubleshootCommand::Tail(const String& file, int numLines, InfoLog& log)
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

	InfoLogLine(log, Console_ForegroundCyan)
	    << "[begin: '" << file << "' line: " << lines-numLines << "]\n";

	for (int k = 0; k < numLines; k++) {
		InfoLogLine(log, Console_ForegroundCyan)
		    <<  "#  ";
		InfoLogLine(log)
		    << ringBuf[k] << '\n';
	}

	text.close();

	InfoLogLine(log, Console_ForegroundCyan)
	    << "[end: '" << file << "' line: " << lines << "]\n\n";

	return numLines;
}

bool TroubleshootCommand::CheckFeatures(InfoLog& log)
{
	Dictionary::Ptr features = new Dictionary;
	std::vector<String> disabled_features;
	std::vector<String> enabled_features;

	if (!FeatureUtility::GetFeatures(disabled_features, true) ||
	    !FeatureUtility::GetFeatures(enabled_features, false)) {
		InfoLogLine(log, 0, LogCritical)
		    << "Failed to collect enabled and/or disabled features. Check\n"
		    << FeatureUtility::GetFeaturesAvailablePath() << '\n'
		    << FeatureUtility::GetFeaturesEnabledPath() << '\n';
		return false;
	}

	for (const String feature : disabled_features)
		features->Set(feature, false);
	for (const String feature : enabled_features)
		features->Set(feature, true);

	InfoLogLine(log)
	    << "Enabled features:\n";
	InfoLogLine(log, Console_ForegroundGreen)
	    << '\t' << boost::algorithm::join(enabled_features, " ") << '\n';
	InfoLogLine(log)
	    << "Disabled features:\n";
	InfoLogLine(log, Console_ForegroundRed)
	    << '\t' << boost::algorithm::join(disabled_features, " ") << '\n';

	if (!features->Get("checker").ToBool())
		InfoLogLine(log, 0, LogWarning)
		    << "checker is disabled, no checks can be run from this instance\n";
	if (!features->Get("mainlog").ToBool())
		InfoLogLine(log, 0, LogWarning)
		    << "mainlog is disabled, please activate it and rerun icinga2\n";
	if (!features->Get("debuglog").ToBool())
		InfoLogLine(log, 0, LogWarning)
		    << "debuglog is disabled, please activate it and rerun icinga2\n";

	return true;
}

void TroubleshootCommand::GetLatestReport(const String& filename, time_t& bestTimestamp, String& bestFilename)
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

bool TroubleshootCommand::PrintCrashReports(InfoLog& log)
{
	String spath = Application::GetLocalStateDir() + "/log/icinga2/crash/report.*";
	time_t bestTimestamp = 0;
	String bestFilename;

	try {
		Utility::Glob(spath, std::bind(&GetLatestReport, _1, std::ref(bestTimestamp),
		    std::ref(bestFilename)), GlobFile);
	}
#ifdef _WIN32
	catch (win32_error &ex) {
		if (int const * err = boost::get_error_info<errinfo_win32_error>(ex)) {
			if (*err != 3) {//Error code for path does not exist
				InfoLogLine(log, 0, LogWarning)
				    << Application::GetLocalStateDir() << "/log/icinga2/crash/ does not exist\n";

				return false;
			}
		}
		InfoLogLine(log, 0, LogWarning)
		    << "Error printing crash reports\n";

		return false;
	}
#else
	catch (...) {
		InfoLogLine(log, 0, LogWarning) << "Error printing crash reports.\n"
		    << "Does " << Application::GetLocalStateDir() << "/log/icinga2/crash/ exist?\n";

		return false;
	}
#endif /*_WIN32*/

	if (!bestTimestamp)
		InfoLogLine(log, Console_ForegroundYellow)
		    << "No crash logs found in " << Application::GetLocalStateDir().CStr() << "/log/icinga2/crash/\n\n";
	else {
		InfoLogLine(log)
		    << "Latest crash report is from " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", Utility::GetTime()) << '\n'
		    << "File: " << bestFilename << "\n\n";
		PrintFile(log, bestFilename);
		InfoLogLine(log)
		    << '\n';
	}

	return true;
}

bool TroubleshootCommand::PrintFile(InfoLog& log, const String& path)
{
	std::ifstream text;
	text.open(path.CStr(), std::ifstream::in);
	if (!text.is_open())
		return false;

	std::string line;

	InfoLogLine(log, Console_ForegroundCyan)
	    << "[begin: '" << path << "']\n";

	while (std::getline(text, line)) {
		InfoLogLine(log, Console_ForegroundCyan)
		    << "#  ";
		InfoLogLine(log)
		    << line << '\n';
	}

	InfoLogLine(log, Console_ForegroundCyan)
	     << "[end: '" << path << "']\n";

	return true;
}

bool TroubleshootCommand::CheckConfig(void)
{
	std::vector<std::string> configs;
	configs.push_back(Application::GetSysconfDir() + "/icinga2/icinga2.conf");

	return DaemonUtility::ValidateConfigFiles(configs, Application::GetObjectsPath());
}

//print is supposed allow the user to print the object file
void TroubleshootCommand::CheckObjectFile(const String& objectfile, InfoLog& log, InfoLog *OFile, const bool objectConsole,
     Dictionary::Ptr& logs, std::set<String>& configs)
{
	InfoLogLine(log)
	     << "Checking object file from " << objectfile << '\n';

	std::fstream fp;
	fp.open(objectfile.CStr(), std::ios_base::in);

	if (!fp.is_open()) {
		InfoLogLine(log, 0, LogWarning)
		     << "Could not open object file.\n";
		return;
	}

	StdioStream::Ptr sfp = new StdioStream(&fp, false);
	String::SizeType typeL = 0, countTotal = 0;

	String message;
	StreamReadContext src;
	StreamReadStatus srs;
	std::map<String, int> type_count;
	bool first = true;

	std::stringstream sStream;

	if (objectConsole)
		InfoLogLine(log, Console_ForegroundBlue)
			<< "\n[begin: objectfile]\n";

	while ((srs = NetString::ReadStringFromStream(sfp, &message, src)) != StatusEof) {
		if (srs != StatusNewItem)
			continue;

		if (objectConsole) {
			ObjectListUtility::PrintObject(std::cout, first, message, type_count, "", "");
		}
		else {
		ObjectListUtility::PrintObject(sStream, first, message, type_count, "", "");
			if (OFile) {
				InfoLogLine(*OFile)
				    << sStream.str();
				sStream.flush();
			}
		}

		Dictionary::Ptr object = JsonDecode(message);
		Dictionary::Ptr properties = object->Get("properties");

		String name = object->Get("name");
		String type = object->Get("type");

		//Find longest typename for padding
		typeL = type.GetLength() > typeL ? type.GetLength() : typeL;
		countTotal++;

		Array::Ptr debug_info = object->Get("debug_info");

		if (debug_info)
			configs.insert(debug_info->Get(0));

		if (Utility::Match(type, "FileLogger")) {
			Dictionary::Ptr debug_hints = object->Get("debug_hints");
			Dictionary::Ptr properties = object->Get("properties");

			ObjectLock olock(properties);
			for (const Dictionary::Pair& kv : properties) {
				if (Utility::Match(kv.first, "path"))
					logs->Set(name, kv.second);
			}
		}
	}

	if (objectConsole)
		InfoLogLine(log, Console_ForegroundBlue)
			<< "\n[end: objectfile]\n";

	if (!countTotal) {
		InfoLogLine(log, 0, LogCritical)
		    << "No objects found in objectfile.\n";
		return;
	}

	//Print objects with count
	InfoLogLine(log)
	    << "Found the " << countTotal << " objects:\n"
	    << "  Type" << std::string(typeL-4, ' ') << " : Count\n";

	for (const Dictionary::Pair& kv : type_count) {
		InfoLogLine(log)
		    << "  " << kv.first << std::string(typeL - kv.first.GetLength(), ' ')
		    << " : " << kv.second << '\n';
	}

	InfoLogLine(log)
	    << '\n';

	TroubleshootCommand::PrintObjectOrigin(log, configs);
}

bool TroubleshootCommand::PrintVarsFile(const String& path, const bool console) {
	if (!console) {
		std::ofstream *ofs = new std::ofstream();
		ofs->open((path+"-vars").CStr(), std::ios::out | std::ios::trunc);
		if (!ofs->is_open())
			return false;
		else
			VariableUtility::PrintVariables(*ofs);
		ofs->close();
	} else
		VariableUtility::PrintVariables(std::cout);

	return true;
}

void TroubleshootCommand::PrintLoggers(InfoLog& log, Dictionary::Ptr& logs)
{
	if (!logs->GetLength()) {
		InfoLogLine(log, 0, LogWarning)
		     << "No loggers found, check whether you enabled any logging features\n";
	} else {
		InfoLogLine(log)
		     << "Getting the last 20 lines of " << logs->GetLength() << " FileLogger objects.\n";

		ObjectLock ulock(logs);
		for (const Dictionary::Pair& kv : logs) {
			InfoLogLine(log)
			     << "Logger " << kv.first << " at path: " << kv.second << '\n';

			if (!Tail(kv.second, 20, log)) {
				InfoLogLine(log, 0, LogWarning)
				     << kv.second << " either does not exist or is empty\n";
			}
		}
	}
}

void TroubleshootCommand::PrintObjectOrigin(InfoLog& log, const std::set<String>& configSet)
{
	InfoLogLine(log)
	     << "The objects origins are:\n";

	for (const String& config : configSet) {
		InfoLogLine(log)
		     << "  " << config << '\n';
	}
}

void TroubleshootCommand::InitParameters(boost::program_options::options_description& visibleDesc,
     boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("console,c", "print to console instead of file")
		("output,o", boost::program_options::value<std::string>(), "path to output file")
		("include-objects", "Print the whole objectfile (like `object list`)")
		("include-vars", "Print all Variables (like `variable list`)")
		;
}

int TroubleshootCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
#ifdef _WIN32 //Dislikes ':' in filenames
	String path = Application::GetLocalStateDir() + "/log/icinga2/troubleshooting-"
	     + Utility::FormatDateTime("%Y-%m-%d_%H-%M-%S", Utility::GetTime()) + ".log";
#else
	String path = Application::GetLocalStateDir() + "/log/icinga2/troubleshooting-"
	     + Utility::FormatDateTime("%Y-%m-%d_%H:%M:%S", Utility::GetTime()) + ".log";
#endif /*_WIN32*/

	InfoLog *log;
	Logger::SetConsoleLogSeverity(LogWarning);

	if (vm.count("output"))
		path = vm["output"].as<std::string>();

	if (vm.count("console")) {
		log = new InfoLog("", true);
	} else {
		log = new InfoLog(path, false);
		if (!log->GetStreamHealth()) {
			Log(LogCritical, "troubleshoot", "Failed to open file to write: " + path);
			delete log;
			return 3;
		}
	}

	String appName = Utility::BaseName(Application::GetArgV()[0]);
	double goTime = Utility::GetTime();

	InfoLogLine(*log)
	    << appName << " -- Troubleshooting help:\n"
	    << "Should you run into problems with Icinga please add this file to your help request\n"
	    << "Started collection at " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", goTime) << "\n";

	InfoLogLine(*log, Console_ForegroundMagenta)
	    << std::string(52, '=') << "\n\n";

	if (appName.GetLength() > 3 && appName.SubStr(0, 3) == "lt-")
		appName = appName.SubStr(3, appName.GetLength() - 3);

	Dictionary::Ptr logs = new Dictionary;

	if (!GeneralInfo(*log, vm) ||
	    !FeatureInfo(*log, vm) ||
	    !ObjectInfo(*log, vm, logs, path) ||
	    !ReportInfo(*log, vm, logs) ||
	    !ConfigInfo(*log, vm)) {
		InfoLogLine(*log, 0, LogCritical)
		    << "Could not recover from critical failure, exiting.\n";

		delete log;
		return 3;
	}

	double endTime = Utility::GetTime();

	InfoLogLine(*log, Console_ForegroundMagenta)
	    << std::string(52, '=') << '\n';
	InfoLogLine(*log, Console_ForegroundGreen)
	    << "Finished collection at " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", endTime)
	    << "\nTook " << Convert::ToString(endTime - goTime) << " seconds\n";

	if (!vm.count("console")) {
		std::cout << "Started collection at " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", goTime) << "\n"
		    << "Finished collection at " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", endTime)
		    << "\nTook " << Convert::ToString(endTime - goTime) << " seconds\n\n";

		std::cout << "General log file: '" << path << "'\n";

		if (vm.count("include-vars"))
			std::cout << "Vars log file: '" << path << "-vars'\n";
		if (vm.count("include-objects"))
			std::cout << "Objects log file: '" << path << "-objects'\n";

		std::cout << "\nPlease compress the files before uploading them,, for example:\n"
	           << "  # tar czf troubleshoot.tar.gz " << path << "*\n";
	}

	delete log;
	return 0;
}
