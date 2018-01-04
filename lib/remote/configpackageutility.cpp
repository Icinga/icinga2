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

#include "remote/configpackageutility.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include "base/scriptglobal.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <fstream>

using namespace icinga;

String ConfigPackageUtility::GetPackageDir()
{
	return Application::GetLocalStateDir() + "/lib/icinga2/api/packages";
}

void ConfigPackageUtility::CreatePackage(const String& name)
{
	String path = GetPackageDir() + "/" + name;

	if (Utility::PathExists(path))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Package already exists."));

	Utility::MkDirP(path, 0700);
	WritePackageConfig(name);
}

void ConfigPackageUtility::DeletePackage(const String& name)
{
	String path = GetPackageDir() + "/" + name;

	if (!Utility::PathExists(path))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Package does not exist."));

	Utility::RemoveDirRecursive(path);
	Application::RequestRestart();
}

std::vector<String> ConfigPackageUtility::GetPackages()
{
	std::vector<String> packages;
	Utility::Glob(GetPackageDir() + "/*", std::bind(&ConfigPackageUtility::CollectDirNames,
		_1, std::ref(packages)), GlobDirectory);
	return packages;
}

void ConfigPackageUtility::CollectDirNames(const String& path, std::vector<String>& dirs)
{
	dirs.emplace_back(Utility::BaseName(path));
}

bool ConfigPackageUtility::PackageExists(const String& name)
{
	return Utility::PathExists(GetPackageDir() + "/" + name);
}

String ConfigPackageUtility::CreateStage(const String& packageName, const Dictionary::Ptr& files)
{
	String stageName = Utility::NewUniqueID();

	String path = GetPackageDir() + "/" + packageName;

	if (!Utility::PathExists(path))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Package does not exist."));

	path += "/" + stageName;

	Utility::MkDirP(path, 0700);
	Utility::MkDirP(path + "/conf.d", 0700);
	Utility::MkDirP(path + "/zones.d", 0700);
	WriteStageConfig(packageName, stageName);

	bool foundDotDot = false;

	if (files) {
		ObjectLock olock(files);
		for (const Dictionary::Pair& kv : files) {
			if (ContainsDotDot(kv.first)) {
				foundDotDot = true;
				break;
			}

			String filePath = path + "/" + kv.first;

			Log(LogInformation, "ConfigPackageUtility")
				<< "Updating configuration file: " << filePath;

			// Pass the directory and generate a dir tree, if it does not already exist
			Utility::MkDirP(Utility::DirName(filePath), 0750);
			std::ofstream fp(filePath.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
			fp << kv.second;
			fp.close();
		}
	}

	if (foundDotDot) {
		Utility::RemoveDirRecursive(path);
		BOOST_THROW_EXCEPTION(std::invalid_argument("Path must not contain '..'."));
	}

	return stageName;
}

void ConfigPackageUtility::WritePackageConfig(const String& packageName)
{
	String stageName = GetActiveStage(packageName);

	String includePath = GetPackageDir() + "/" + packageName + "/include.conf";
	std::ofstream fpInclude(includePath.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpInclude << "include \"*/include.conf\"\n";
	fpInclude.close();

	String activePath = GetPackageDir() + "/" + packageName + "/active.conf";
	std::ofstream fpActive(activePath.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpActive << "if (!globals.contains(\"ActiveStages\")) {\n"
		<< "  globals.ActiveStages = {}\n"
		<< "}\n"
		<< "\n"
		<< "if (globals.contains(\"ActiveStageOverride\")) {\n"
		<< "  var arr = ActiveStageOverride.split(\":\")\n"
		<< "  if (arr[0] == \"" << packageName << "\") {\n"
		<< "    if (arr.len() < 2) {\n"
		<< "      log(LogCritical, \"Config\", \"Invalid value for ActiveStageOverride\")\n"
		<< "    } else {\n"
		<< "      ActiveStages[\"" << packageName << "\"] = arr[1]\n"
		<< "    }\n"
		<< "  }\n"
		<< "}\n"
		<< "\n"
		<< "if (!ActiveStages.contains(\"" << packageName << "\")) {\n"
		<< "  ActiveStages[\"" << packageName << "\"] = \"" << stageName << "\"\n"
		<< "}\n";
	fpActive.close();
}

void ConfigPackageUtility::WriteStageConfig(const String& packageName, const String& stageName)
{
	String path = GetPackageDir() + "/" + packageName + "/" + stageName + "/include.conf";
	std::ofstream fp(path.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fp << "include \"../active.conf\"\n"
		<< "if (ActiveStages[\"" << packageName << "\"] == \"" << stageName << "\") {\n"
		<< "  include_recursive \"conf.d\"\n"
		<< "  include_zones \"" << packageName << "\", \"zones.d\"\n"
		<< "}\n";
	fp.close();
}

void ConfigPackageUtility::ActivateStage(const String& packageName, const String& stageName)
{
	String activeStagePath = GetPackageDir() + "/" + packageName + "/active-stage";
	std::ofstream fpActiveStage(activeStagePath.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpActiveStage << stageName;
	fpActiveStage.close();

	WritePackageConfig(packageName);
}

void ConfigPackageUtility::TryActivateStageCallback(const ProcessResult& pr, const String& packageName, const String& stageName, bool reload)
{
	String logFile = GetPackageDir() + "/" + packageName + "/" + stageName + "/startup.log";
	std::ofstream fpLog(logFile.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpLog << pr.Output;
	fpLog.close();

	String statusFile = GetPackageDir() + "/" + packageName + "/" + stageName + "/status";
	std::ofstream fpStatus(statusFile.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpStatus << pr.ExitStatus;
	fpStatus.close();

	/* validation went fine, activate stage and reload */
	if (pr.ExitStatus == 0) {
		{
			boost::mutex::scoped_lock lock(GetStaticMutex());
			ActivateStage(packageName, stageName);
		}

		if (reload)
			Application::RequestRestart();
	} else {
		Log(LogCritical, "ConfigPackageUtility")
			<< "Config validation failed for package '"
			<< packageName << "' and stage '" << stageName << "'.";
	}
}

void ConfigPackageUtility::AsyncTryActivateStage(const String& packageName, const String& stageName, bool reload)
{
	VERIFY(Application::GetArgC() >= 1);

	// prepare arguments
	Array::Ptr args = new Array();
	args->Add(Application::GetExePath(Application::GetArgV()[0]));
	args->Add("daemon");
	args->Add("--validate");
	args->Add("--define");
	args->Add("ActiveStageOverride=" + packageName + ":" + stageName);

	Process::Ptr process = new Process(Process::PrepareCommand(args));
	process->SetTimeout(300);
	process->Run(std::bind(&TryActivateStageCallback, _1, packageName, stageName, reload));
}

void ConfigPackageUtility::DeleteStage(const String& packageName, const String& stageName)
{
	String path = GetPackageDir() + "/" + packageName + "/" + stageName;

	if (!Utility::PathExists(path))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Stage does not exist."));

	if (GetActiveStage(packageName) == stageName)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Active stage cannot be deleted."));

	Utility::RemoveDirRecursive(path);
}

std::vector<String> ConfigPackageUtility::GetStages(const String& packageName)
{
	std::vector<String> stages;
	Utility::Glob(GetPackageDir() + "/" + packageName + "/*", std::bind(&ConfigPackageUtility::CollectDirNames, _1, std::ref(stages)), GlobDirectory);
	return stages;
}

String ConfigPackageUtility::GetActiveStage(const String& packageName)
{
	String path = GetPackageDir() + "/" + packageName + "/active-stage";

	std::ifstream fp;
	fp.open(path.CStr());

	String stage;
	std::getline(fp, stage.GetData());

	fp.close();

	if (fp.fail())
		return "";

	return stage.Trim();
}


std::vector<std::pair<String, bool> > ConfigPackageUtility::GetFiles(const String& packageName, const String& stageName)
{
	std::vector<std::pair<String, bool> > paths;
	Utility::GlobRecursive(GetPackageDir() + "/" + packageName + "/" + stageName, "*", std::bind(&ConfigPackageUtility::CollectPaths, _1, std::ref(paths)), GlobDirectory | GlobFile);

	return paths;
}

void ConfigPackageUtility::CollectPaths(const String& path, std::vector<std::pair<String, bool> >& paths)
{
#ifndef _WIN32
	struct stat statbuf;
	int rc = lstat(path.CStr(), &statbuf);
	if (rc < 0)
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("lstat")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(path));

	paths.emplace_back(path, S_ISDIR(statbuf.st_mode));
#else /* _WIN32 */
	struct _stat statbuf;
	int rc = _stat(path.CStr(), &statbuf);
	if (rc < 0)
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("_stat")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(path));

	paths.emplace_back(path, ((statbuf.st_mode & S_IFMT) == S_IFDIR));
#endif /* _WIN32 */
}

bool ConfigPackageUtility::ContainsDotDot(const String& path)
{
	std::vector<String> tokens;
	boost::algorithm::split(tokens, path, boost::is_any_of("/\\"));

	for (const String& part : tokens) {
		if (part == "..")
			return true;
	}

	return false;
}

bool ConfigPackageUtility::ValidateName(const String& name)
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

boost::mutex& ConfigPackageUtility::GetStaticMutex()
{
	static boost::mutex mutex;
	return mutex;
}
