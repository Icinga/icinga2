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

using namespace icinga;

static String GetIcingaInstallDir(void)
{
	char szFileName[MAX_PATH];
	if (!GetModuleFileName(NULL, szFileName, sizeof(szFileName)))
		return "";
	return Utility::DirName(Utility::DirName(szFileName));
}

static void ExecuteCommand(const String& command)
{
	std::cerr << "Executing command: " << command << std::endl;
	system(command.CStr());
}

static void ExecuteIcingaCommand(const String& args)
{
	ExecuteCommand("\"" + GetIcingaInstallDir() + "\\sbin\\icinga2.exe\" " + args);
}

static void CopyConfigFile(const String& installDir, const String& sourceConfigPath, size_t skelPrefixLength)
{
	String relativeConfigPath = sourceConfigPath.SubStr(installDir.GetLength() + skelPrefixLength);

	String targetConfigPath = installDir + relativeConfigPath;

	if (!Utility::PathExists(targetConfigPath)) {
		Utility::MkDirP(Utility::DirName(targetConfigPath), 0700);
		Utility::CopyFile(sourceConfigPath, targetConfigPath);
	}
}

static int InstallIcinga(void)
{
	String installDir = GetIcingaInstallDir();

	ExecuteCommand("icacls \"" + installDir + "\" /grant *S-1-5-20:(oi)(ci)m");
	ExecuteCommand("icacls \"" + installDir + "\\etc\" /inheritance:r /grant:r *S-1-5-20:(oi)(ci)m *S-1-5-32-544:(oi)(ci)f");

	Utility::MkDirP(installDir + "/etc/icinga2/pki", 0700);
	Utility::MkDirP(installDir + "/var/cache/icinga2", 0700);
	Utility::MkDirP(installDir + "/var/lib/icinga2/pki", 0700);
	Utility::MkDirP(installDir + "/var/lib/icinga2/agent/inventory", 0700);
	Utility::MkDirP(installDir + "/var/lib/icinga2/api/config", 0700);
	Utility::MkDirP(installDir + "/var/lib/icinga2/api/log", 0700);
	Utility::MkDirP(installDir + "/var/lib/icinga2/api/zones", 0700);
	Utility::MkDirP(installDir + "/var/lib/icinga2/api/zones", 0700);
	Utility::MkDirP(installDir + "/var/log/icinga2/compat/archive", 0700);
	Utility::MkDirP(installDir + "/var/log/icinga2/crash", 0700);
	Utility::MkDirP(installDir + "/var/run/icinga2/cmd", 0700);
	Utility::MkDirP(installDir + "/var/spool/icinga2/perfdata", 0700);
	Utility::MkDirP(installDir + "/var/spool/icinga2/tmp", 0700);

	String skelDir = "/share/skel";
	Utility::GlobRecursive(installDir + skelDir, "*", boost::bind(&CopyConfigFile, installDir, _1, skelDir.GetLength()), GlobFile);

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
