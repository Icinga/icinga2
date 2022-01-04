/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include <string>
#include <vector>
#include <fstream>
#include <direct.h>
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <sys/types.h>
#include <sys/stat.h>

static std::string GetIcingaInstallPath(void)
{
	char szFileName[MAX_PATH];
	if (!GetModuleFileName(nullptr, szFileName, sizeof(szFileName)))
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

static std::string GetIcingaDataPath(void)
{
	char path[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, path)))
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
		struct _stat statbuf;
		if (_stat(spath.c_str(), &statbuf) < 0 && errno == ENOENT)
			MkDir(path.substr(0, pos));
	}
}

static std::string GetNSISInstallPath(void)
{
	HKEY hKey;
	//TODO: Change hardcoded key
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Icinga Development Team\\ICINGA2", 0,
		KEY_QUERY_VALUE | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS) {
		BYTE pvData[MAX_PATH];
		DWORD cbData = sizeof(pvData) - 1;
		DWORD lType;
		if (RegQueryValueEx(hKey, nullptr, nullptr, &lType, pvData, &cbData) == ERROR_SUCCESS && lType == REG_SZ) {
			pvData[cbData] = '\0';

			return (char *)pvData;
		}

		RegCloseKey(hKey);
	}

	return "";
}

static bool CopyDirectory(const std::string& source, const std::string& destination)
{
	// SHFileOperation requires file names to be terminated with two \0s
	std::string tmpSource = source + std::string(1, '\0');
	std::string tmpDestination = destination + std::string(1, '\0');

	SHFILEOPSTRUCT fop;
	fop.wFunc = FO_COPY;
	fop.pFrom = tmpSource.c_str();
	fop.pTo = tmpDestination.c_str();
	fop.fFlags = FOF_NO_UI;

	return (SHFileOperation(&fop) == 0);
}

static bool DeleteDirectory(const std::string& dir)
{
	// SHFileOperation requires file names to be terminated with two \0s
	std::string tmpDir = dir + std::string(1, '\0');

	SHFILEOPSTRUCT fop;
	fop.wFunc = FO_DELETE;
	fop.pFrom = tmpDir.c_str();
	fop.fFlags = FOF_NO_UI;

	return (SHFileOperation(&fop) == 0);
}

static int UpgradeNSIS(void)
{
	std::string installPath = GetNSISInstallPath();

	if (installPath.empty())
		return 0;

	std::string uninstallerPath = installPath + "\\uninstall.exe";

	if (!PathExists(uninstallerPath))
		return 0;

	std::string dataPath = GetIcingaDataPath();

	if (dataPath.empty())
		return 1;

	bool moveUserData = !PathExists(dataPath);

	/* perform open heart surgery on the user's data dirs - yay */
	if (moveUserData) {
		MkDir(dataPath.c_str());

		std::string oldNameEtc = installPath + "\\etc";
		std::string newNameEtc = dataPath + "\\etc";
		if (!CopyDirectory(oldNameEtc, newNameEtc))
			return 1;

		std::string oldNameVar = installPath + "\\var";
		std::string newNameVar = dataPath + "\\var";
		if (!CopyDirectory(oldNameVar, newNameVar))
			return 1;
	}

	ExecuteCommand(uninstallerPath, "/S _?=" + installPath);

	_unlink(uninstallerPath.c_str());

	if (moveUserData) {
		std::string oldNameEtc = installPath + "\\etc";
		if (!DeleteDirectory(oldNameEtc))
			return 1;

		std::string oldNameVar = installPath + "\\var";
		if (!DeleteDirectory(oldNameVar))
			return 1;

		_rmdir(installPath.c_str());
	}

	return 0;
}

static int InstallIcinga(void)
{
	std::string installDir = GetIcingaInstallPath();
	std::string skelDir = installDir + "\\share\\skel";
	std::string dataDir = GetIcingaDataPath();

	if (!PathExists(dataDir)) {
		std::string sourceDir = skelDir + std::string(1, '\0');
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
		MkDirP(dataDir + "/var/lib/icinga2/certs");
		MkDirP(dataDir + "/var/lib/icinga2/certificate-requests");
		MkDirP(dataDir + "/var/lib/icinga2/agent/inventory");
		MkDirP(dataDir + "/var/lib/icinga2/api/config");
		MkDirP(dataDir + "/var/lib/icinga2/api/log");
		MkDirP(dataDir + "/var/lib/icinga2/api/zones");
		MkDirP(dataDir + "/var/log/icinga2/compat/archive");
		MkDirP(dataDir + "/var/log/icinga2/crash");
		MkDirP(dataDir + "/var/run/icinga2/cmd");
		MkDirP(dataDir + "/var/spool/icinga2/perfdata");
		MkDirP(dataDir + "/var/spool/icinga2/tmp");
	}

	// Upgrade from versions older than 2.13 by making the windowseventlog feature available,
	// enable it by default and disable the old mainlog feature.
	if (!PathExists(dataDir + "/etc/icinga2/features-available/windowseventlog.conf")) {
		// Disable the old mainlog feature as it is replaced by windowseventlog by default.
		std::string mainlogEnabledFile = dataDir + "/etc/icinga2/features-enabled/mainlog.conf";
		if (PathExists(mainlogEnabledFile)) {
			if (DeleteFileA(mainlogEnabledFile.c_str()) == 0) {
				throw std::runtime_error("deleting '" + mainlogEnabledFile + "' failed");
			}
		}

		// Install the new windowseventlog feature. As features-available/windowseventlog.conf is used as a marker file,
		// copy it as the last step, so that this is run again should the upgrade be interrupted.
		for (const auto& d : {"features-enabled", "features-available"}) {
			std::string sourceFile = skelDir + "/etc/icinga2/" + d + "/windowseventlog.conf";
			std::string destinationFile = dataDir + "/etc/icinga2/" + d + "/windowseventlog.conf";

			if (CopyFileA(sourceFile.c_str(), destinationFile.c_str(), false) == 0) {
				throw std::runtime_error("copying '" + sourceFile + "' to '" + destinationFile + "' failed");
			}
		}
	}

	// TODO: In Icinga 2.14, rename features-available/mainlog.conf to mainlog.conf.deprecated
	//       so that it's no longer listed as an available feature.

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
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	//AllocConsole();
	int rc;

	if (strcmp(lpCmdLine, "install") == 0) {
		rc = InstallIcinga();
	} else if (strcmp(lpCmdLine, "uninstall") == 0) {
		rc = UninstallIcinga();
	} else if (strcmp(lpCmdLine, "upgrade-nsis") == 0) {
		rc = UpgradeNSIS();
	} else {
		MessageBox(nullptr, "This application should only be run by the MSI installer package.", "Icinga 2 Installer", MB_ICONWARNING);
		rc = 1;
	}

	//::Sleep(3000s);

	return rc;
}
