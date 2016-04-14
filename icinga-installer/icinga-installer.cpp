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

#include <string>
#include <vector>
#include <fstream>
#include <direct.h>
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlobj.h>

static std::string GetIcingaInstallPath(void)
{
	char szFileName[MAX_PATH];
	if (!GetModuleFileName(NULL, szFileName, sizeof(szFileName)))
		return "";

	if (!PathRemoveFileSpec(szFileName))
		return "";

	if (!PathRemoveFileSpec(szFileName))
		return "";

	return szFileName;
}

static bool ExecuteCommand(const std::string& app, const std::string& arguments)
{
	SHELLEXECUTEINFO sei = {};
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.lpFile = app.c_str();
	sei.lpParameters = arguments.c_str();
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

static bool ExecuteIcingaCommand(const std::string& arguments)
{
	return ExecuteCommand(GetIcingaInstallPath() + "\\sbin\\icinga2.exe", arguments);
}

static std::string DirName(const std::string& path)
{
	char *spath = strdup(path.c_str());

	if (!PathRemoveFileSpec(spath)) {
		free(spath);
		throw std::runtime_error("PathRemoveFileSpec failed");
	}

	std::string result = spath;

	free(spath);

	return result;
}

static bool PathExists(const std::string& path)
{
	struct _stat statbuf;
	return (_stat(path.c_str(), &statbuf) >= 0);
}

static void CopyFile(const std::string& source, const std::string& target)
{
	std::ifstream ifs(source.c_str(), std::ios::binary);
	std::ofstream ofs(target.c_str(), std::ios::binary | std::ios::trunc);

	ofs << ifs.rdbuf();
}

static std::string GetIcingaDataPath(void)
{
	char path[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path)))
		throw std::runtime_error("SHGetFolderPath failed");
	return std::string(path) + "\\icinga2";
}

static void MkDir(const std::string& path)
{
	if (mkdir(path.c_str()) < 0 && errno != EEXIST)
		throw std::runtime_error("mkdir failed");
}

static void MkDirP(const std::string& path)
{
	size_t pos = 0;

	while (pos != std::string::npos) {
		pos = path.find_first_of("/\\", pos + 1);

		std::string spath = path.substr(0, pos + 1);
		struct stat statbuf;
		if (stat(spath.c_str(), &statbuf) < 0 && errno == ENOENT)
			MkDir(path.substr(0, pos));
	}
}

static void CopyConfigFile(const std::string& installDir, const std::string& sourceConfigPath, size_t skelPrefixLength)
{
	std::string relativeConfigPath = sourceConfigPath.substr(skelPrefixLength);

	std::string targetConfigPath = installDir + relativeConfigPath;

	if (!PathExists(targetConfigPath)) {
		MkDirP(DirName(targetConfigPath));
		CopyFile(sourceConfigPath, targetConfigPath);
	}
}

static std::string GetNSISInstallPath(void)
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

static void CollectPaths(std::vector<std::string>& paths, const std::string& path)
{
	paths.push_back(path);
}

static bool MoveDirectory(const std::string& source, const std::string& destination)
{
	if (!MoveFileEx(source.c_str(), destination.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) {
		// SHFileOperation requires file names to be terminated with two \0s
		std::string tmpSource = source + std::string(1, '\0');
		std::string tmpDestination = destination + std::string(1, '\0');

		SHFILEOPSTRUCT fop;
		fop.wFunc = FO_COPY;
		fop.pFrom = tmpSource.c_str();
		fop.pTo = tmpDestination.c_str();
		fop.fFlags = FOF_NO_UI;
		if (SHFileOperation(&fop) != 0)
			return false;
	}

	return true;
}

static int UpgradeNSIS(void)
{
	std::string installPath = GetNSISInstallPath();

	if (installPath.empty())
		return 0;

	std::string uninstallerPath = installPath + "\\uninstall.exe";

	if (!PathExists(uninstallerPath))
		return 0;

	ExecuteCommand(uninstallerPath, "/S _?=" + installPath);

	_unlink(uninstallerPath.c_str());

	std::string dataPath = GetIcingaDataPath();

	/* perform open heart surgery on the user's data dirs - yay */
	if (!PathExists(dataPath)) {
		MkDir(dataPath.c_str());

		std::string oldNameEtc = installPath + "\\etc";
		std::string newNameEtc = dataPath + "\\etc";
		if (!MoveDirectory(oldNameEtc, newNameEtc))
			return 1;

		std::string oldNameVar = installPath + "\\var";
		std::string newNameVar = dataPath + "\\var";
		if (!MoveDirectory(oldNameVar, newNameVar))
			return 1;

		_rmdir(installPath.c_str());
	}	

	return 0;
}

static int InstallIcinga(void)
{
	std::string installDir = GetIcingaInstallPath();
	std::string dataDir = GetIcingaDataPath();

	if (!PathExists(dataDir)) {
		std::string sourceDir = installDir + "\\share\\skel" + std::string(1, '\0');
		std::string destinationDir = dataDir + std::string(1, '\0');

		SHFILEOPSTRUCT fop;
		fop.wFunc = FO_COPY;
		fop.pFrom = sourceDir.c_str();
		fop.pTo = destinationDir.c_str();
		fop.fFlags = FOF_NO_UI | FOF_NOCOPYSECURITYATTRIBS;

		if (SHFileOperation(&fop) != 0)
			return 1;

		MkDirP(dataDir + "/etc/icinga2/pki");
		MkDirP(dataDir + "/var/cache/icinga2");
		MkDirP(dataDir + "/var/lib/icinga2/pki");
		MkDirP(dataDir + "/var/lib/icinga2/agent/inventory");
		MkDirP(dataDir + "/var/lib/icinga2/api/config");
		MkDirP(dataDir + "/var/lib/icinga2/api/log");
		MkDirP(dataDir + "/var/lib/icinga2/api/zones");
		MkDirP(dataDir + "/var/lib/icinga2/api/zones");
		MkDirP(dataDir + "/var/log/icinga2/compat/archive");
		MkDirP(dataDir + "/var/log/icinga2/crash");
		MkDirP(dataDir + "/var/run/icinga2/cmd");
		MkDirP(dataDir + "/var/spool/icinga2/perfdata");
		MkDirP(dataDir + "/var/spool/icinga2/tmp");
	}

	ExecuteCommand("icacls", "\"" + dataDir + "\" /grant *S-1-5-20:(oi)(ci)m");
	ExecuteCommand("icacls", "\"" + dataDir + "\\etc\" /inheritance:r /grant:r *S-1-5-20:(oi)(ci)m *S-1-5-32-544:(oi)(ci)f");

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
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	//AllocConsole();

	int rc;

	if (strcmp(lpCmdLine, "install") == 0) {
		rc = InstallIcinga();
	} else if (strcmp(lpCmdLine, "uninstall") == 0) {
		rc = UninstallIcinga();
	} else if (strcmp(lpCmdLine, "upgrade-nsis") == 0) {
		rc = UpgradeNSIS();
	} else {
		MessageBox(NULL, "This application should only be run by the MSI installer package.", "Icinga 2 Installer", MB_ICONWARNING);
		rc = 1;
	}

	//::Sleep(3000s);

	return rc;
}
