/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "remote/configmoduleutility.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include "base/scriptglobal.hpp"
#include "base/utility.hpp"
#include "boost/foreach.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <fstream>

using namespace icinga;

String ConfigModuleUtility::GetModuleDir(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/api/modules";
}

void ConfigModuleUtility::CreateModule(const String& name)
{
	String path = GetModuleDir() + "/" + name;

	if (Utility::PathExists(path))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Module already exists."));

	Utility::MkDirP(path, 0700);
	WriteModuleConfig(name);
}

void ConfigModuleUtility::DeleteModule(const String& name)
{
	String path = GetModuleDir() + "/" + name;

	if (!Utility::PathExists(path))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Module does not exist."));

	Utility::RemoveDirRecursive(path);
	Application::RequestRestart();
}

std::vector<String> ConfigModuleUtility::GetModules(void)
{
	std::vector<String> modules;
	Utility::Glob(GetModuleDir() + "/*", boost::bind(&ConfigModuleUtility::CollectDirNames, _1, boost::ref(modules)), GlobDirectory);
	return modules;
}

void ConfigModuleUtility::CollectDirNames(const String& path, std::vector<String>& dirs)
{
	String name = Utility::BaseName(path);
	dirs.push_back(name);
}

String ConfigModuleUtility::CreateStage(const String& moduleName, const Dictionary::Ptr& files)
{
	String stageName = Utility::NewUniqueID();

	String path = GetModuleDir() + "/" + moduleName;

	if (!Utility::PathExists(path))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Module does not exist."));

	path += "/" + stageName;

	Utility::MkDirP(path, 0700);
	WriteStageConfig(moduleName, stageName);

	ObjectLock olock(files);
	BOOST_FOREACH(const Dictionary::Pair& kv, files) {
		if (ContainsDotDot(kv.first))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Path must not contain '..'."));

		String filePath = path + "/" + kv.first;

		Log(LogInformation, "ConfigModuleUtility")
		    << "Updating configuration file: " << filePath;

		//pass the directory and generate a dir tree, if not existing already
		Utility::MkDirP(Utility::DirName(filePath), 0750);
		std::ofstream fp(filePath.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
		fp << kv.second;
		fp.close();
	}

	return stageName;
}

void ConfigModuleUtility::WriteModuleConfig(const String& moduleName)
{
	String stageName = GetActiveStage(moduleName);

	String includePath = GetModuleDir() + "/" + moduleName + "/include.conf";
	std::ofstream fpInclude(includePath.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpInclude << "include \"*/include.conf\"\n";
	fpInclude.close();

	String activePath = GetModuleDir() + "/" + moduleName + "/active.conf";
	std::ofstream fpActive(activePath.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpActive << "if (!globals.contains(\"ActiveStages\")) {\n"
		 << "  globals.ActiveStages = {}\n"
		 << "}\n"
		 << "\n"
		 << "if (globals.contains(\"ActiveStageOverride\")) {\n"
		 << "  var arr = ActiveStageOverride.split(\":\")\n"
		 << "  if (arr[0] == \"" << moduleName << "\") {\n"
		 << "    if (arr.len() < 2) {\n"
		 << "      log(LogCritical, \"Config\", \"Invalid value for ActiveStageOverride\")\n"
		 << "    } else {\n"
		 << "      ActiveStages[\"" << moduleName << "\"] = arr[1]\n"
		 << "    }\n"
		 << "  }\n"
		 << "}\n"
		 << "\n"
		 << "if (!ActiveStages.contains(\"" << moduleName << "\")) {\n"
		 << "  ActiveStages[\"" << moduleName << "\"] = \"" << stageName << "\"\n"
		 << "}\n";
	fpActive.close();
}

void ConfigModuleUtility::WriteStageConfig(const String& moduleName, const String& stageName)
{
	String path = GetModuleDir() + "/" + moduleName + "/" + stageName + "/include.conf";
	std::ofstream fp(path.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fp << "include \"../active.conf\"\n"
	   << "if (ActiveStages[\"" << moduleName << "\"] == \"" << stageName << "\") {\n"
	   << "  include_recursive \"conf.d\"\n"
	   << "  include_zones \"" << moduleName << "\", \"zones.d\"\n"
	   << "}\n";
	fp.close();
}

void ConfigModuleUtility::ActivateStage(const String& moduleName, const String& stageName)
{
	String activeStagePath = GetModuleDir() + "/" + moduleName + "/active-stage";
	std::ofstream fpActiveStage(activeStagePath.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpActiveStage << stageName;
	fpActiveStage.close();

	WriteModuleConfig(moduleName);
}

void ConfigModuleUtility::TryActivateStageCallback(const ProcessResult& pr, const String& moduleName, const String& stageName)
{
	String logFile = GetModuleDir() + "/" + moduleName + "/" + stageName + "/startup.log";
	std::ofstream fpLog(logFile.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpLog << pr.Output;
	fpLog.close();

	String statusFile = GetModuleDir() + "/" + moduleName + "/" + stageName + "/status";
	std::ofstream fpStatus(statusFile.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpStatus << pr.ExitStatus;
	fpStatus.close();

	/* validation went fine, activate stage and reload */
	if (pr.ExitStatus == 0) {
		ActivateStage(moduleName, stageName);
		Application::RequestRestart();
	} else {
		Log(LogCritical, "ConfigModuleUtility")
		    << "Config validation failed for module '"
		    << moduleName << "' and stage '" << stageName << "'.";
	}
}

void ConfigModuleUtility::AsyncTryActivateStage(const String& moduleName, const String& stageName)
{
	// prepare arguments
	Array::Ptr args = new Array();
	args->Add(Application::GetExePath("icinga2"));
	args->Add("daemon");
	args->Add("--validate");
	args->Add("--define");
	args->Add("ActiveStageOverride=" + moduleName + ":" + stageName);

	Process::Ptr process = new Process(Process::PrepareCommand(args));
	process->SetTimeout(300);
	process->Run(boost::bind(&TryActivateStageCallback, _1, moduleName, stageName));
}

void ConfigModuleUtility::DeleteStage(const String& moduleName, const String& stageName)
{
	String path = GetModuleDir() + "/" + moduleName + "/" + stageName;

	if (!Utility::PathExists(path))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Stage does not exist."));

	if (GetActiveStage(moduleName) == stageName)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Active stage cannot be deleted."));

	Utility::RemoveDirRecursive(path);
}

std::vector<String> ConfigModuleUtility::GetStages(const String& moduleName)
{
	std::vector<String> stages;
	Utility::Glob(GetModuleDir() + "/" + moduleName + "/*", boost::bind(&ConfigModuleUtility::CollectDirNames, _1, boost::ref(stages)), GlobDirectory);
	return stages;
}

String ConfigModuleUtility::GetActiveStage(const String& moduleName)
{
	String path = GetModuleDir() + "/" + moduleName + "/active-stage";

	std::ifstream fp;
	fp.open(path.CStr());

	String stage;
	std::getline(fp, stage.GetData());
	stage.Trim();

	fp.close();

	if (fp.fail())
		return "";

	return stage;
}


std::vector<std::pair<String, bool> > ConfigModuleUtility::GetFiles(const String& moduleName, const String& stageName)
{
	std::vector<std::pair<String, bool> > paths;
	Utility::GlobRecursive(GetModuleDir() + "/" + moduleName + "/" + stageName, "*", boost::bind(&ConfigModuleUtility::CollectPaths, _1, boost::ref(paths)), GlobDirectory | GlobFile);

	return paths;
}

void ConfigModuleUtility::CollectPaths(const String& path, std::vector<std::pair<String, bool> >& paths)
{
#ifndef _WIN32
	struct stat statbuf;
	int rc = lstat(path.CStr(), &statbuf);
	if (rc < 0)
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("lstat")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(path));
#else /* _WIN32 */
	struct _stat statbuf;
	int rc = _stat(path.CStr(), &statbuf);
	if (rc < 0)
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("_stat")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(path));
#endif /* _WIN32 */

	paths.push_back(std::make_pair(path, S_ISDIR(statbuf.st_mode)));
}

bool ConfigModuleUtility::ContainsDotDot(const String& path)
{
	std::vector<String> tokens;
	boost::algorithm::split(tokens, path, boost::is_any_of("/\\"));

	BOOST_FOREACH(const String& part, tokens) {
		if (part == "..")
			return true;
	}

	return false;
}

bool ConfigModuleUtility::ValidateName(const String& name)
{
	if (name.IsEmpty())
		return false;

	/* check for path injection */
	if (ContainsDotDot(name))
		return false;

	boost::regex expr("^[^a-zA-Z0-9_\\-]*$", boost::regex::icase);
	boost::smatch what;
	return (!boost::regex_search(name.GetData(), what, expr));
}


