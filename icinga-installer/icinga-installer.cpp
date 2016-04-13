/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "base/utility.hpp"
#include "base/application.hpp"
#include <boost/foreach.hpp>
#include <shellapi.h>

using namespace icinga;

static String GetIcingaInstallPath(void)
{
	char szFileName[MAX_PATH];
	if (!GetModuleFileName(NULL, szFileName, sizeof(szFileName)))
		return "";
	return Utility::DirName(Utility::DirName(szFileName));
}

static bool ExecuteCommand(const String& app, const String& arguments)
{
	SHELLEXECUTEINFO sei = {};
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.lpFile = app.CStr();
	sei.lpParameters = arguments.CStr();
	sei.nShow = SW_HIDE;
	if (!ShellExecuteEx(&sei))
		return false;
	
	if (!sei.hProcess)
		return false;

	WaitForSingleObject(sei.hProcess, INFINITE);

	DWORD exitCode;
	BOOL res = GetExitCodeProcess(sei.hProcess, &exitCode);
	CloseHandle(sei.hProcess);

	if (!res)
		return false;

	return exitCode == 0;
}

static bool ExecuteIcingaCommand(const String& arguments)
{
	return ExecuteCommand(GetIcingaInstallPath() + "\\sbin\\icinga2.exe", arguments);
}

static void CopyConfigFile(const String& installDir, const String& sourceConfigPath, size_t skelPrefixLength)
{
	String relativeConfigPath = sourceConfigPath.SubStr(skelPrefixLength);

	String targetConfigPath = installDir + relativeConfigPath;

	if (!Utility::PathExists(targetConfigPath)) {
		Utility::MkDirP(Utility::DirName(targetConfigPath), 0700);
		Utility::CopyFile(sourceConfigPath, targetConfigPath);
	}
}

static String GetNSISInstallPath(void)
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Icinga Development Team\\ICINGA2", 0,
		KEY_QUERY_VALUE | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS) {
		BYTE pvData[MAX_PATH];
		DWORD cbData = sizeof(pvData) - 1;
		DWORD lType;
		if (RegQueryValueEx(hKey, NULL, NULL, &lType, pvData, &cbData) == ERROR_SUCCESS && lType == REG_SZ) {
			pvData[cbData] = '\0';

			return (char *)pvData;
		}

		RegCloseKey(hKey);
	}

	return "";
}

static void CollectPaths(std::vector<String>& paths, const String& path)
{
	paths.push_back(path);
}

static bool MoveDirectory(const String& source, const String& destination)
{
	if (!MoveFileEx(source.CStr(), destination.CStr(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) {
		// SHFileOperation requires file names to be terminated with two \0s
		String tmpSource = source + String(1, '\0');
		String tmpDestination = destination + String(1, '\0');

		SHFILEOPSTRUCT fop;
		fop.wFunc = FO_COPY;
		fop.pFrom = tmpSource.CStr();
		fop.pTo = tmpDestination.CStr();
		fop.fFlags = FOF_NO_UI;
		if (SHFileOperation(&fop) != 0)
			return false;

		std::vector<String> paths;
		paths.push_back(source);
		Utility::GlobRecursive(source, "*", boost::bind(&CollectPaths, boost::ref(paths), _1), GlobDirectory);
		Utility::GlobRecursive(source, "*", boost::bind(&CollectPaths, boost::ref(paths), _1), GlobFile);

		std::reverse(paths.begin(), paths.end());

		BOOST_FOREACH(const String& path, paths) {
			(void)MoveFileEx(path.CStr(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
		}
	}

	return true;
}

static int UpgradeNSIS(void)
{
	String installPath = GetNSISInstallPath();

	if (installPath.IsEmpty())
		return 0;

	if (!Utility::PathExists(installPath + "\\uninstall.exe"))
		return 0;

	ExecuteCommand(installPath + "\\uninstall.exe", "/S");

	String dataPath = Utility::GetIcingaDataPath();

	/* perform open heart surgery on the user's data dirs - yay */
	if (!Utility::PathExists(dataPath)) {
		Utility::MkDirP(dataPath, 0700);

		String oldNameEtc = installPath + "\\etc";
		String newNameEtc = dataPath + "\\etc";
		if (!MoveDirectory(oldNameEtc, newNameEtc))
			return 1;

		String oldNameVar = installPath + "\\var";
		String newNameVar = dataPath + "\\var";
		if (!MoveDirectory(oldNameVar, newNameVar))
			return 1;
	}	

	return 0;
}

static int InstallIcinga(void)
{
	if (UpgradeNSIS() != 0)
		return 1;

	String installDir = GetIcingaInstallPath();
	String dataDir = Utility::GetIcingaDataPath();

	Utility::MkDirP(dataDir, 0700);

	ExecuteCommand("icacls", "\"" + dataDir + "\" /grant *S-1-5-20:(oi)(ci)m");
	ExecuteCommand("icacls", "\"" + dataDir + "\\etc\" /inheritance:r /grant:r *S-1-5-20:(oi)(ci)m *S-1-5-32-544:(oi)(ci)f");

	Utility::MkDirP(dataDir + "/etc/icinga2/pki", 0700);
	Utility::MkDirP(dataDir + "/var/cache/icinga2", 0700);
	Utility::MkDirP(dataDir + "/var/lib/icinga2/pki", 0700);
	Utility::MkDirP(dataDir + "/var/lib/icinga2/agent/inventory", 0700);
	Utility::MkDirP(dataDir + "/var/lib/icinga2/api/config", 0700);
	Utility::MkDirP(dataDir + "/var/lib/icinga2/api/log", 0700);
	Utility::MkDirP(dataDir + "/var/lib/icinga2/api/zones", 0700);
	Utility::MkDirP(dataDir + "/var/lib/icinga2/api/zones", 0700);
	Utility::MkDirP(dataDir + "/var/log/icinga2/compat/archive", 0700);
	Utility::MkDirP(dataDir + "/var/log/icinga2/crash", 0700);
	Utility::MkDirP(dataDir + "/var/run/icinga2/cmd", 0700);
	Utility::MkDirP(dataDir + "/var/spool/icinga2/perfdata", 0700);
	Utility::MkDirP(dataDir + "/var/spool/icinga2/tmp", 0700);

	String skelDir = "/share/skel";
	Utility::GlobRecursive(installDir + skelDir, "*", boost::bind(&CopyConfigFile, dataDir, _1, installDir.GetLength() + skelDir.GetLength()), GlobFile);

	ExecuteIcingaCommand("--scm-install daemon");

	return 0;
}

static int UninstallIcinga(void)
{
	ExecuteIcingaCommand("--scm-uninstall");

	return 0;
}

/**
* Entry point for the installer application.
*/
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	/* must be called before using any other libbase functions */
	Application::InitializeBase();

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	//AllocConsole();

	int rc;

	if (strcmp(lpCmdLine, "install") == 0) {
		rc = InstallIcinga();
	} else if (strcmp(lpCmdLine, "uninstall") == 0) {
		rc = UninstallIcinga();
	} else {
		MessageBox(NULL, "This application should only be run by the MSI installer package.", "Icinga 2 Installer", MB_ICONWARNING);
		rc = 1;
	}

	//Utility::Sleep(3);

	Application::Exit(rc);
}
