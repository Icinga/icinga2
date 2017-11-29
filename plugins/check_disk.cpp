/******************************************************************************
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
#include <set>
#include <Shlwapi.h>
#include <iostream>
#include <math.h>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "check_disk.h"

#define VERSION 1.1

namespace po = boost::program_options;

static BOOL debug = FALSE;

INT wmain(INT argc, WCHAR **argv) 
{
	std::vector<drive> vDrives;
	printInfoStruct printInfo{ };
	po::variables_map vm;

	INT ret;

	ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	printInfo.warn.legal = !printInfo.warn.legal;
	printInfo.crit.legal = !printInfo.crit.legal;

	if (printInfo.drives.empty())
		ret = check_drives(vDrives, printInfo.exclude_drives);
	else
		ret = check_drives(vDrives, printInfo);
	
	if (ret != -1)
		return ret;

	for (std::vector<drive>::iterator it = vDrives.begin(); it != vDrives.end(); ++it) {
		if (!getDriveSpaceValues(*it, printInfo.unit)) {
			std::wcout << "Failed to access drive at " << it->name << '\n';
			return 3;
		}
	}

	return printOutput(printInfo, vDrives);
}

static INT parseArguments(INT ac, WCHAR **av, po::variables_map& vm, printInfoStruct& printInfo) 
{
	WCHAR namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	WCHAR *progName = PathFindFileName(namePath);

	po::options_description desc("Options");

	desc.add_options()
		("help,h", "Print usage message and exit")
		("version,V", "Print version and exit")
		("debug,d", "Verbose/Debug output")
		("warning,w", po::wvalue<std::wstring>(), "Warning threshold")
		("critical,c", po::wvalue<std::wstring>(), "Critical threshold")
		("path,p", po::wvalue<std::vector<std::wstring>>()->multitoken(), "Declare explicitly which drives to check (default checks all)")
		("exclude_device,x", po::wvalue<std::vector<std::wstring>>()->multitoken(), "Exclude these drives from check")
		("exclude-type,X", po::wvalue<std::vector<std::wstring>>()->multitoken(), "Exclude partition types (ignored)")
		("iwarning,W", po::wvalue<std::wstring>(), "Warning threshold for inodes (ignored)")
		("icritical,K", po::wvalue<std::wstring>(), "Critical threshold for inodes (ignored)")
		("unit,u", po::wvalue<std::wstring>(), "Assign unit possible are: B, kB, MB, GB, TB")
		("show-used,U", "Show used space instead of the free space")
		("megabytes,m", "use megabytes, overridden by -unit")
		;

	po::basic_command_line_parser<WCHAR> parser(ac, av);

	try {
		po::store(
			parser
			.options(desc)
			.style(
			po::command_line_style::unix_style |
			po::command_line_style::allow_long_disguise)
			.run(),
			vm);
		vm.notify();
	} catch (std::exception& e) {
		std::cout << e.what() << '\n' << desc << '\n';
		return 3;
	}

	if (vm.count("help")) {
		std::wcout << progName << " Help\n\tVersion: " << VERSION << '\n';
		wprintf(
			L"%s is a simple program to check a machines disk space usage.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tDISK WARNING 29GB | disk=29GB;50%%;5;0;120\n\n"
			L"\"DISK\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"23.8304%%\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value.\n"
			L"This program will also print out additional performance data disk by disk\n\n"
			L"%s' exit codes denote the following:\n\n"
			L" 0\tOK,\n\tNo Thresholds were broken or the programs check part was not executed\n"
			L" 1\tWARNING,\n\tThe warning, but not the critical threshold was broken\n"
			L" 2\tCRITICAL,\n\tThe critical threshold was broken\n"
			L" 3\tUNKNOWN, \n\tThe program experienced an internal or input error\n\n"
			L"Threshold syntax:\n\n"
			L"-w THRESHOLD\n"
			L"warn if threshold is broken, which means VALUE < THRESHOLD\n\n"
			L"-w !THRESHOLD\n"
			L"inverts threshold check, VALUE > THRESHOLD (analogous to above)\n\n"
			L"-w [THR1-THR2]\n"
			L"warn is VALUE is inside the range spanned by THR1 and THR2\n\n"
			L"-w ![THR1-THR2]\n"
			L"warn if VALUE is outside the range spanned by THR1 and THR2\n\n"
			L"-w THRESHOLD%%\n"
			L"if the plugin accepts percentage based thresholds those will be used.\n"
			L"Does nothing if the plugin does not accept percentages, or only uses\n"
			L"percentage thresholds. Ranges can be used with \"%%\", but both range values need\n"
			L"to end with a percentage sign.\n\n"
			L"All of these options work with the critical threshold \"-c\" too."
			, progName);
		std::cout << '\n';
		return 0;
	}

	if (vm.count("version"))
		std::cout << "Version: " << VERSION << '\n';

	if (vm.count("warning")) {
		try {
			printInfo.warn = threshold(vm["warning"].as<std::wstring>());
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}
	if (vm.count("critical")) {
		try {
			printInfo.crit = threshold(vm["critical"].as<std::wstring>());
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}
	
	if (vm.count("path")) 
		printInfo.drives = vm["path"].as<std::vector<std::wstring>>();

	if (vm.count("exclude_device"))
		printInfo.exclude_drives = vm["exclude_device"].as<std::vector<std::wstring>>();

	if (vm.count("unit")) {
		try {
			printInfo.unit = parseBUnit(vm["unit"].as<std::wstring>());
		} catch (std::invalid_argument) {
			std::wcout << "Unknown unit Type " << vm["unit"].as<std::wstring>() << '\n';
			return 3;
		}
	} else {
		if (vm.count("megabytes"))
			printInfo.unit = BunitMB;
		else
			printInfo.unit = BunitB;
	}

	printInfo.showUsed = vm.count("show-used");

	if (vm.count("debug"))
		debug = TRUE;

	return -1;
}

static INT printOutput(printInfoStruct& printInfo, std::vector<drive>& vDrives) 
{
	if (debug)
		std::wcout << "Constructing output string\n";

	std::vector<std::wstring> wsDrives, wsPerf;
	std::wstring unit = BunitStr(printInfo.unit);

	state state = OK;

	std::wstring output = L"DISK OK - free space:";

	if (printInfo.showUsed)
		output = L"DISK OK - used space:";
	
	double tCap = 0, tFree = 0, tUsed = 0;

	for (std::vector<drive>::iterator it = vDrives.begin(); it != vDrives.end(); it++) {
		tCap += it->cap;
		tFree += it->free;
		tUsed += it->used;

		if (printInfo.showUsed)
		{
			wsDrives.push_back(it->name + L" " + removeZero(it->used) + L" " + unit + L" (" +
				removeZero(std::round(it->used / it->cap * 100.0)) + L"%); ");

			wsPerf.push_back(L" " + it->name + L"=" + removeZero(it->used) + unit + L";" +
				printInfo.warn.pString(it->cap) + L";" + printInfo.crit.pString(it->cap) + L";0;"
				+ removeZero(it->cap));

			if (printInfo.crit.set && !printInfo.crit.rend(it->used, it->cap))
				state = CRITICAL;

			if (state == OK && printInfo.warn.set && !printInfo.warn.rend(it->used, it->cap))
				state = WARNING;
		}
		else {
			wsDrives.push_back(it->name + L" " + removeZero(it->free) + L" " + unit + L" (" +
				removeZero(std::round(it->free / it->cap * 100.0)) + L"%); ");

			wsPerf.push_back(L" " + it->name + L"=" + removeZero(it->free) + unit + L";" +
				printInfo.warn.pString(it->cap) + L";" + printInfo.crit.pString(it->cap) + L";0;"
				+ removeZero(it->cap));

			if ( printInfo.crit.rend(it->free, it->cap))
				state = CRITICAL;

			if (state == OK && printInfo.warn.rend(it->free, it->cap))
				state = WARNING;
		}
	}

	if (state == WARNING) {
		output = L"DISK WARNING - free space:";
		
		if (printInfo.showUsed)
			output = L"DISK WARNING - used space:";
	}

	if (state == CRITICAL) {
		output = L"DISK CRITICAL - free space:";

		if (printInfo.showUsed)
			output = L"DISK CRITICAL - used space:";
	}

	std::wcout << output;

	if (vDrives.size() > 1) {
		if (printInfo.showUsed) {
			std::wcout << "Total " << (printInfo.showUsed ? tUsed : tFree) << unit
			    << " (" << removeZero(std::round(tUsed / tCap * 100.0)) << "%); ";
		}
	}

	for (std::vector<std::wstring>::const_iterator it = wsDrives.begin(); it != wsDrives.end(); it++)
		std::wcout << *it;

	std::wcout << "|";

	for (std::vector<std::wstring>::const_iterator it = wsPerf.begin(); it != wsPerf.end(); it++)
		std::wcout << *it;

	std::wcout << '\n';
	return state;
}

static INT check_drives(std::vector<drive>& vDrives, std::vector<std::wstring>& vExclude_Drives)
{
	DWORD dwResult, dwSize = 0, dwVolumePathNamesLen = MAX_PATH + 1;
	WCHAR szLogicalDrives[1024], szVolumeName[MAX_PATH], *szVolumePathNames = NULL;
	HANDLE hVolume = NULL;
	std::wstring wsLogicalDrives;
	size_t volumeNameEnd = 0;

	std::set<std::wstring> sDrives;

	if (debug)
		std::wcout << "Getting logic drive string (includes network drives)\n";

	dwResult = GetLogicalDriveStrings(MAX_PATH, szLogicalDrives);
	if (dwResult > MAX_PATH)
		goto die;
	if (debug)
		std::wcout << "Splitting string intoo single drive names\n";

	LPTSTR szSingleDrive = szLogicalDrives;
	while (*szSingleDrive) {
		std::wstring drname = szSingleDrive;
		sDrives.insert(drname);
		szSingleDrive += wcslen(szSingleDrive) + 1;
		if (debug)
			std::wcout << "Got: " << drname << '\n';
	}

	if (debug)
		std::wcout << "Getting volume mountpoints (includes NTFS folders)\n"
		    << "Getting first volume\n";

	hVolume = FindFirstVolume(szVolumeName, MAX_PATH);
	if (hVolume == INVALID_HANDLE_VALUE)
		goto die;

	if (debug)
		std::wcout << "Traversing through list of drives\n";

	while (GetLastError() != ERROR_NO_MORE_FILES) {
		if (debug)
			std::wcout << "Path name for " << szVolumeName << "= \"";
		volumeNameEnd = wcslen(szVolumeName) - 1;
		szVolumePathNames = reinterpret_cast<WCHAR*>(new WCHAR[dwVolumePathNamesLen]);

		while (!GetVolumePathNamesForVolumeName(szVolumeName, szVolumePathNames, dwVolumePathNamesLen, 
			    &dwVolumePathNamesLen)) {
			if (GetLastError() != ERROR_MORE_DATA)
				break;
			delete[] reinterpret_cast<WCHAR*>(szVolumePathNames);
			szVolumePathNames = reinterpret_cast<WCHAR*>(new WCHAR[dwVolumePathNamesLen]);

		}
		if (debug)
			std::wcout << szVolumePathNames << "\"\n";

		sDrives.insert(std::wstring(szVolumePathNames));
		FindNextVolume(hVolume, szVolumeName, MAX_PATH);
	}
	if (debug)
		std::wcout << "Creating vector from found volumes, ignoring cd drives etc.:\n";
	for (std::set<std::wstring>::iterator it = sDrives.begin(); it != sDrives.end(); ++it) {
		UINT type = GetDriveType(it->c_str());
		if (type == DRIVE_FIXED || type == DRIVE_REMOTE) {
			if (debug)
				std::wcout << "\t" << *it << '\n';
			vDrives.push_back(drive(*it));
		}
	}

	FindVolumeClose(hVolume);
	if (szVolumePathNames)
		delete[] reinterpret_cast<WCHAR*>(szVolumePathNames);

	if (!vExclude_Drives.empty()) {
		if (debug)
			std::wcout << "Removing excluded drives\n";

		BOOST_FOREACH(const std::wstring wsDriveName, vExclude_Drives)
		{
			vDrives.erase(std::remove_if(vDrives.begin(), vDrives.end(), 
			    std::bind(checkName, _1, wsDriveName + L'\\')), vDrives.end());
		}
	}
	return -1;

die:
	if (hVolume)
		FindVolumeClose(hVolume);
	die();
	return 3;
}

static INT check_drives(std::vector<drive>& vDrives, printInfoStruct& printInfo) 
{
	if (!printInfo.exclude_drives.empty()) {
		if (debug)
			std::wcout << "Removing excluded drives from user input drives\n";
		BOOST_FOREACH(std::wstring wsDrive, printInfo.exclude_drives)
		{
			printInfo.drives.erase(std::remove(printInfo.drives.begin(), printInfo.drives.end(), wsDrive),
			    printInfo.drives.end());
		}
	}

	if (debug)
		std::wcout << "Parsing user input drive names\n";

	for (std::vector<std::wstring>::iterator it = printInfo.drives.begin();
			it != printInfo.drives.end(); ++it) {
		if (it->at(it->length() - 1) != *L"\\")
			it->append(L"\\");
		if (std::wstring::npos == it->find(L":\\")) {
			std::wcout << "A \":\" is required after the drive name of " << *it << '\n';
			return 3;
		}
		if (debug)
			std::wcout << "Added " << *it << '\n';
		vDrives.push_back(drive(*it));
	}
	return -1;
}

static BOOL getDriveSpaceValues(drive& drive, const Bunit& unit)
{
	if (debug)
		std::wcout << "Getting free and used disk space for drive " << drive.name << '\n';

	ULARGE_INTEGER tempFree, tempTotal, tempUsed;

	if (!GetDiskFreeSpaceEx(drive.name.c_str(), NULL, &tempTotal, &tempFree)) {
		return FALSE;
	}

	tempUsed.QuadPart = tempTotal.QuadPart - tempFree.QuadPart;

	if (debug)
		std::wcout << "\tcap: " << tempFree.QuadPart << '\n';

	drive.cap = round((tempTotal.QuadPart / pow(1024.0, unit)));

	if (debug)
		std::wcout << "\tAfter conversion: " << drive.cap << '\n'
		<< "\tfree: " << tempFree.QuadPart << '\n';

	drive.free = round((tempFree.QuadPart / pow(1024.0, unit)));

	if (debug)
		std::wcout << "\tAfter conversion: " << drive.free << '\n' 
		    << "\tused: " << tempUsed.QuadPart << '\n';

	drive.used = round((tempUsed.QuadPart / pow(1024.0, unit)));

	if (debug)
		std::wcout << "\tAfter conversion: " << drive.used << '\n' << '\n';

	return TRUE;
}

static bool checkName(const drive& d, const std::wstring& s)
{
	return (s == d.name);
}

