// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
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

/* Strips the whitespace appended in the WiX ExeCommand to protect trailing
 * backslashes, as well as stray quotes and trailing backslashes themselves.
 */
static std::string TrimField(std::string value)
{
	while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '"' || value.back() == '\\'))
		value.pop_back();

	while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '"'))
		value.erase(0, 1);

	return value;
}

/* Expands %VARIABLE% references, so that a data directory may be given as e.g.
 * %PROGRAMDATA%\icinga2 and keep following the environment instead of freezing the resolved path.
 */
static std::string ExpandEnvVars(const std::string& text)
{
	if (text.find('%') == std::string::npos)
		return text;

	DWORD len = ExpandEnvironmentStrings(text.c_str(), nullptr, 0);
	if (len == 0)
		return text;

	std::vector<char> expanded(len);
	if (ExpandEnvironmentStrings(text.c_str(), expanded.data(), len) == 0)
		return text;

	return expanded.data();
}

/* Every instance records what it was installed with, so that upgrades and uninstalls - which are not
 * passed any MSI properties - still know the data directory and the service name they belong to.
 *
 * The MSI itself writes these values through the per-instance marker component; it also removes them
 * again when that instance is uninstalled. Packages from before instance support used a single flat
 * key, which is kept in sync for the default instance so that downgrading to such a package still
 * finds its settings.
 */
static const char *l_LegacySettingsKeyPath = "SOFTWARE\\Icinga GmbH\\Icinga 2";
static const char *l_InstancesKeyPath = "SOFTWARE\\Icinga GmbH\\Icinga 2\\Instances";

static const char *l_DefaultInstanceId = "Instance1";
static const char *l_DefaultServiceName = "icinga2";
static const char *l_DefaultEventLogSource = "Icinga 2";

static std::string SettingsKeyPath(const std::string& instanceId)
{
	return std::string(l_InstancesKeyPath) + "\\" + instanceId;
}

static std::string ReadRegistryString(const std::string& keyPath, const char *name)
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return "";

	std::string result;
	BYTE pvData[1024];
	DWORD cbData = sizeof(pvData) - 1;
	DWORD lType;
	if (RegQueryValueEx(hKey, name, nullptr, &lType, pvData, &cbData) == ERROR_SUCCESS && lType == REG_SZ) {
		pvData[cbData] = '\0';
		result = (char *)pvData;
	}

	RegCloseKey(hKey);

	return result;
}

static void WriteRegistryString(const std::string& keyPath, const char *name, const std::string& value)
{
	HKEY hKey;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, nullptr, 0,
		KEY_SET_VALUE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
		throw std::runtime_error("failed to create registry key " + keyPath);

	LONG res;
	if (value.empty())
		res = RegDeleteValue(hKey, name);
	else
		res = RegSetValueEx(hKey, name, 0, REG_SZ,
			(const BYTE *)value.c_str(), (DWORD)value.size() + 1);

	RegCloseKey(hKey);

	if (res != ERROR_SUCCESS && !(value.empty() && res == ERROR_FILE_NOT_FOUND))
		throw std::runtime_error(std::string("failed to persist registry value ") + name);
}

static std::string ReadPersistedString(const std::string& instanceId, const char *name)
{
	std::string result = ReadRegistryString(SettingsKeyPath(instanceId), name);

	if (result.empty() && instanceId == l_DefaultInstanceId)
		result = ReadRegistryString(l_LegacySettingsKeyPath, name);

	return result;
}

/* The file the installed binaries use to find out which instance they belong to. It lives next to
 * them instead of in the registry or the environment so that icinga2.exe run from a console targets
 * the same instance as the service does. The MSI removes it again on uninstall.
 */
static std::string InstanceIniPath(const std::string& installDir)
{
	return installDir + "\\instance.ini";
}

static std::string ReadInstanceIni(const std::string& installDir, const std::string& name)
{
	std::ifstream fp(InstanceIniPath(installDir));
	if (!fp.good())
		return "";

	std::string line;
	while (std::getline(fp, line)) {
		size_t pos = line.find('=');
		if (pos == std::string::npos)
			continue;

		if (line.compare(0, pos, name) == 0)
			return TrimField(line.substr(pos + 1));
	}

	return "";
}

static void WriteInstanceIni(const std::string& installDir, const std::string& instanceId,
	const std::string& serviceName, const std::string& dataDir, const std::string& eventLogSource)
{
	std::string path = InstanceIniPath(installDir);
	std::string tmpPath = path + ".tmp";

	{
		std::ofstream fp(tmpPath, std::ios::trunc);

		fp << "; Written by the Icinga 2 installer. Identifies the instance these binaries belong to.\n"
			<< "[instance]\n"
			<< "Id=" << instanceId << "\n"
			<< "ServiceName=" << serviceName << "\n"
			<< "DataDir=" << dataDir << "\n"
			<< "EventLogSource=" << eventLogSource << "\n"
			<< "InstallRoot=" << installDir << "\n";

		if (!fp.good())
			throw std::runtime_error("failed to write " + tmpPath);
	}

	/* Replace it atomically, so that an interrupted upgrade cannot leave a truncated file behind. */
	if (!MoveFileEx(tmpPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
		throw std::runtime_error("failed to replace " + path);
}

static std::string WideToNarrow(const wchar_t *str)
{
	int len = WideCharToMultiByte(CP_ACP, 0, str, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 0)
		return "";

	std::string result(len - 1, '\0');
	WideCharToMultiByte(CP_ACP, 0, str, -1, &result[0], len, nullptr, nullptr);
	return result;
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

static int InstallIcinga(std::string dataDir, std::string serviceName, std::string instanceId,
	std::string eventLogSource)
{
	std::string installDir = GetIcingaInstallPath();
	std::string skelDir = installDir + "\\share\\skel";

	if (instanceId.empty())
		instanceId = l_DefaultInstanceId;

	/* Resolution order: MSI property -> value persisted by a previous install -> built-in default.
	 * The MSI passes concrete values for every instance, so the latter two are only a safety net.
	 */
	if (dataDir.empty())
		dataDir = ReadPersistedString(instanceId, "DataDir");
	if (serviceName.empty())
		serviceName = ReadPersistedString(instanceId, "ServiceName");
	if (eventLogSource.empty())
		eventLogSource = ReadPersistedString(instanceId, "EventLogSource");

	std::string defaultDataDir = GetIcingaDataPath();

	if (dataDir.empty())
		dataDir = defaultDataDir;
	if (serviceName.empty())
		serviceName = l_DefaultServiceName;
	if (eventLogSource.empty())
		eventLogSource = l_DefaultEventLogSource;

	/* The value is persisted as given, so that a reference like %PROGRAMDATA%\icinga2 keeps
	 * following the environment; the file system operations below work on the expanded path.
	 */
	std::string rawDataDir = dataDir;
	dataDir = ExpandEnvVars(dataDir);

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
		for (const std::string& d : {"features-enabled", "features-available"}) {
			std::string sourceFile = skelDir + "/etc/icinga2/" + d + "/windowseventlog.conf";
			std::string destinationFile = dataDir + "/etc/icinga2/" + d + "/windowseventlog.conf";

			if (CopyFileA(sourceFile.c_str(), destinationFile.c_str(), false) == 0) {
				throw std::runtime_error("copying '" + sourceFile + "' to '" + destinationFile + "' failed");
			}
		}
	}

	// TODO: In Icinga 2.14, rename features-available/mainlog.conf to mainlog.conf.deprecated
	//       so that it's no longer listed as an available feature.

	if (!ExecuteCommand("icacls", "\"" + dataDir + "\" /grant *S-1-5-20:(oi)(ci)m")){
		throw std::runtime_error("failed to set ACLs for " + dataDir);
	}
	if (!ExecuteCommand("icacls", "\"" + dataDir + "\\etc\" /inheritance:r /grant:r *S-1-5-20:(oi)(ci)m *S-1-5-32-544:(oi)(ci)f")) {
		throw std::runtime_error("failed to set ACLs for " + dataDir + "\\etc");
	}
	if (!ExecuteCommand("icacls", "\"" + dataDir + "\\var\" /inheritance:r /grant:r *S-1-5-20:(oi)(ci)m *S-1-5-32-544:(oi)(ci)f")) {
		throw std::runtime_error("failed to set ACLs for " + dataDir + "\\var");
	}

	/* Tell the binaries which instance they belong to. This is what makes icinga2.exe use the right
	 * data directory no matter whether it is started by the service control manager or from a console.
	 */
	WriteInstanceIni(installDir, instanceId, serviceName, rawDataDir, eventLogSource);

	/* icinga2.exe --scm-install copies both of these into the service's "Environment" registry value,
	 * so that the service process gets them from the service control manager. The child process
	 * inherits them from us. The binaries expand %VARIABLE% references themselves.
	 */
	SetEnvironmentVariable("ICINGA2_DATA_PATH", rawDataDir.c_str());
	SetEnvironmentVariable("ICINGA2_INSTALL_PATH", installDir.c_str());

	std::string scmArgs = "--scm-install --scm-name \"" + serviceName + "\" daemon";

	ExecuteIcingaCommand(scmArgs);

	/* Packages from before instance support read the flat key; keep it in sync for the default
	 * instance, still removing values that are back at the defaults so that a default installation
	 * leaves no traces. The per-instance key itself is written and removed by the MSI.
	 */
	if (instanceId == l_DefaultInstanceId) {
		WriteRegistryString(l_LegacySettingsKeyPath, "DataDir", dataDir != defaultDataDir ? rawDataDir : "");
		WriteRegistryString(l_LegacySettingsKeyPath, "ServiceName",
			serviceName != l_DefaultServiceName ? serviceName : "");
	}

	return 0;
}

static int UninstallIcinga(std::string instanceId)
{
	if (instanceId.empty())
		instanceId = l_DefaultInstanceId;

	std::string installDir = GetIcingaInstallPath();

	std::string dataDir = ReadPersistedString(instanceId, "DataDir");
	std::string serviceName = ReadPersistedString(instanceId, "ServiceName");

	/* Both the registry key and instance.ini are removed by the MSI during this very session, so
	 * fall back to whichever is still there.
	 */
	if (dataDir.empty())
		dataDir = ReadInstanceIni(installDir, "DataDir");
	if (serviceName.empty())
		serviceName = ReadInstanceIni(installDir, "ServiceName");

	if (!dataDir.empty())
		SetEnvironmentVariable("ICINGA2_DATA_PATH", dataDir.c_str());
	if (!installDir.empty())
		SetEnvironmentVariable("ICINGA2_INSTALL_PATH", installDir.c_str());

	std::string scmArgs = "--scm-uninstall";
	if (!serviceName.empty())
		scmArgs += " --scm-name \"" + serviceName + "\"";

	ExecuteIcingaCommand(scmArgs);

	return 0;
}

static bool SamePath(const std::string& left, const std::string& right)
{
	return !left.empty() && _stricmp(TrimField(left).c_str(), TrimField(right).c_str()) == 0;
}

/* Whether any instance other than the one being uninstalled was installed into installDir. */
static bool InstallPathClaimedByOtherInstance(const std::string& instanceId, const std::string& installDir)
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, l_InstancesKeyPath, 0, KEY_ENUMERATE_SUB_KEYS, &hKey) != ERROR_SUCCESS)
		return false;

	bool claimed = false;

	for (DWORD i = 0; !claimed; i++) {
		char szName[256];
		DWORD cchName = sizeof(szName);

		if (RegEnumKeyEx(hKey, i, szName, &cchName, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
			break;

		if (instanceId == szName)
			continue;

		claimed = SamePath(ReadRegistryString(SettingsKeyPath(szName), "InstallLocation"), installDir);
	}

	RegCloseKey(hKey);

	return claimed;
}

static bool DeleteDirectoryWithRetries(const std::string& dir)
{
	/* The service was deleted moments ago; a process that has not fully exited yet can still hold a
	 * handle on sbin.
	 */
	for (int attempt = 0; attempt < 5; attempt++) {
		if (DeleteDirectory(dir))
			return true;

		Sleep(1000);
	}

	return false;
}

/* Windows Installer only removes a component's files when no other product uses that component. An
 * instance transform regenerates the GUID only of components authored with MultiInstance="yes", and
 * the components CPack generates for the payload are not marked as such, so all instances share
 * them. Uninstalling one instance while others remain therefore leaves its installation directory
 * behind. Remove it here, but only what unambiguously belongs to this instance.
 */
static int CleanupIcinga(std::string instanceId, const std::string& installDir,
	const std::string& menuFolder)
{
	if (instanceId.empty())
		instanceId = l_DefaultInstanceId;

	if (!installDir.empty() && installDir.find('\\') != std::string::npos && installDir.size() > 3
		&& !InstallPathClaimedByOtherInstance(instanceId, installDir)) {
		/* Only the leftovers of a shared component look like this: had Windows Installer removed the
		 * files, icinga2.exe would be gone. This is what keeps the cleanup from touching a directory
		 * the installer is still responsible for.
		 */
		if (PathExists(installDir + "\\sbin\\icinga2.exe"))
			DeleteDirectoryWithRetries(installDir);
	}

	if (!menuFolder.empty()) {
		char programs[MAX_PATH];

		/* Only ever a folder directly below the common start menu, never the start menu itself. */
		if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_COMMON_PROGRAMS, nullptr, 0, programs))
			&& !SamePath(menuFolder, programs) && SamePath(DirName(menuFolder), programs)
			&& PathExists(menuFolder)) {
			DeleteDirectoryWithRetries(menuFolder);
		}
	}

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

	int argc = 0;
	LPWSTR *argvW = CommandLineToArgvW(GetCommandLineW(), &argc);

	std::vector<std::string> args;
	if (argvW) {
		for (int i = 1; i < argc; i++)
			args.push_back(WideToNarrow(argvW[i]));
		LocalFree(argvW);
	}

	/* Missing arguments are tolerated and fall back to the default instance, so that a mismatched
	 * pair of WiX patch and executable degrades to the single instance behaviour instead of failing.
	 */
	auto arg = [&args](size_t i) { return args.size() > i ? TrimField(args[i]) : std::string(); };

	try {
		if (!args.empty() && args[0] == "install") {
			rc = InstallIcinga(arg(1), arg(2), arg(3), arg(4));
		} else if (!args.empty() && args[0] == "uninstall") {
			rc = UninstallIcinga(arg(1));
		} else if (!args.empty() && args[0] == "cleanup") {
			rc = CleanupIcinga(arg(1), arg(2), arg(3));
		} else if (!args.empty() && args[0] == "upgrade-nsis") {
			rc = UpgradeNSIS();
		} else {
			MessageBox(nullptr, "This application should only be run by the MSI installer package.", "Icinga 2 Installer", MB_ICONWARNING);
			rc = 1;
		}
	} catch (const std::exception&) {
		/* Deferred custom actions run non-interactively in the system context, so a message box would
		 * block on a desktop nobody sees. The non-zero exit code shows up in the MSI log instead.
		 */
		rc = 1;
	}

	//::Sleep(3000s);

	return rc;
}
