/******************************************************************************
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
#include <Windows.h>
#include <set>
#include <Shlwapi.h>
#include <vector>
#include <iostream>
#include <math.h>

#include "thresholds.h"

#include "boost/program_options.hpp"

#define VERSION 1.0

namespace po = boost::program_options;

using std::cout; using std::endl; using std::set;
using std::vector; using std::wstring; using std::wcout;

static BOOL debug = FALSE;

struct drive 
{
	wstring name;
	double cap, free;
	drive(wstring p)
		: name(p)
	{}
};

struct printInfoStruct 
{
	threshold warn, crit;
	vector<wstring> drives;
	Bunit unit;
};

static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(printInfoStruct&, vector<drive>&);
static int check_drives(vector<drive>&);
static int check_drives(vector<drive>&, printInfoStruct&);
static bool getFreeAndCap(drive&, const Bunit&);

int wmain(int argc, wchar_t **argv) 
{
	vector<drive> vDrives;
	printInfoStruct printInfo{ };
	po::variables_map vm;

	int ret;

	ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	printInfo.warn.legal = !printInfo.warn.legal;
	printInfo.crit.legal = !printInfo.crit.legal;

	if (printInfo.drives.empty())
		ret = check_drives(vDrives);
	else
		ret = check_drives(vDrives, printInfo);
	
	if (ret != -1)
		return ret;

	for (vector<drive>::iterator it = vDrives.begin(); it != vDrives.end(); ++it) {
		if (!getFreeAndCap(*it, printInfo.unit)) {
			wcout << L"Failed to access drive at " << it->name << endl;
			return 3;
		}
	}

	return printOutput(printInfo, vDrives);
}

int parseArguments(int ac, wchar_t **av, po::variables_map& vm, printInfoStruct& printInfo) 
{
	wchar_t namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	wchar_t *progName = PathFindFileName(namePath);

	po::options_description desc("Options");

	desc.add_options()
		("help,h", "print usage message and exit")
		("version,V", "print version and exit")
		("debug,d", "Verbose/Debug output")
		("warning,w", po::wvalue<wstring>(), "warning threshold")
		("critical,c", po::wvalue<wstring>(), "critical threshold")
		("path,p", po::wvalue<vector<std::wstring>>()->multitoken(), "declare explicitly which drives to check (default checks all)")
		("unit,u", po::wvalue<wstring>(), "assign unit possible are: B, kB, MB, GB, TB")
		;

	po::basic_command_line_parser<wchar_t> parser(ac, av);

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
		cout << e.what() << endl << desc << endl;
		return 3;
	}

	if (vm.count("help")) {
		wcout << progName << " Help\n\tVersion: " << VERSION << endl;
		wprintf(
			L"%s is a simple program to check a machines free disk space.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tDISK WARNING 29GB|disk=29GB;50%%;5;0;120\n\n"
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
		cout << endl;
		return 0;
	}

	if (vm.count("version"))
		cout << "Version: " << VERSION << endl;

	if (vm.count("warning")) {
		try {
			printInfo.warn = threshold(vm["warning"].as<wstring>());
		} catch (std::invalid_argument& e) {
			cout << e.what() << endl;
			return 3;
		}
	}
	if (vm.count("critical")) {
		try {
			printInfo.crit = threshold(vm["critical"].as<wstring>());
		} catch (std::invalid_argument& e) {
			cout << e.what() << endl;
			return 3;
		}
	}
	
	if (vm.count("path")) 
		printInfo.drives = vm["path"].as<vector<wstring>>();

	if (vm.count("unit")) {
		try {
			printInfo.unit = parseBUnit(vm["unit"].as<wstring>());
		} catch (std::invalid_argument) {
			wcout << L"Unknown unit Type " << vm["unit"].as<wstring>() << endl;
			return 3;
		}
	} else
		printInfo.unit = BunitB;

	if (vm.count("debug"))
		debug = TRUE;

	return -1;
}

int printOutput(printInfoStruct& printInfo, vector<drive>& vDrives) 
{
	if (debug)
		wcout << L"Constructing output string" << endl;
	state state = OK;
	double tCap = 0, tFree = 0;
	std::wstringstream perf, prePerf;
	wstring unit = BunitStr(printInfo.unit);

	for (vector<drive>::iterator it = vDrives.begin(); it != vDrives.end(); ++it) {
		tCap += it->cap; tFree += it->free;
		perf << std::fixed << L" " << it->name << L"=" << removeZero(it->free) << unit << L";"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;" << removeZero(tCap);
	}

	prePerf << L" | disk=" << removeZero(tFree) << unit << L";" << printInfo.warn.pString() << L";"
		<< printInfo.crit.pString() << L";0;" << removeZero(tCap);

	if (printInfo.warn.perc) {
		if (printInfo.warn.rend((tFree / tCap) * 100.0))
			state = WARNING;
	} else {
		if (printInfo.warn.rend(tFree))
			state = WARNING;
	}

	if (printInfo.crit.perc) {
		if (printInfo.crit.rend((tFree / tCap) * 100.0))
			state = CRITICAL;
	} else {
		if (printInfo.crit.rend(tFree))
			state = CRITICAL;
	}

	switch (state) {
	case OK:
		wcout << L"DISK OK " << std::fixed << removeZero(tFree) << unit << prePerf.str() << perf.str() << endl;
		break;
	case WARNING:
		wcout << L"DISK WARNING " << std::fixed << removeZero(tFree) << unit << prePerf.str() << perf.str() << endl;
		break;
	case CRITICAL:
		wcout << L"DISK CRITICAL " << std::fixed << removeZero(tFree) << unit << prePerf.str() << perf.str() << endl;
		break;
	}

	return state;
}

int check_drives(vector<drive>& vDrives) 
{
	DWORD dwResult, dwSize = 0, dwVolumePathNamesLen = MAX_PATH + 1;
	wchar_t szLogicalDrives[1024], szVolumeName[MAX_PATH], *szVolumePathNames;
	HANDLE hVolume;
	wstring wsLogicalDrives;
	size_t volumeNameEnd = 0;

	set<wstring> sDrives;

	if (debug)
		wcout << L"Getting logic drive string (includes network drives)" << endl;

	dwResult = GetLogicalDriveStrings(MAX_PATH, szLogicalDrives);
	if (dwResult < 0 || dwResult > MAX_PATH) 
		goto die;
	if (debug)
		wcout << L"Splitting string into single drive names" << endl;

	LPTSTR szSingleDrive = szLogicalDrives;
	while (*szSingleDrive) {
		wstring drname = szSingleDrive;
		sDrives.insert(drname);
		szSingleDrive += wcslen(szSingleDrive) + 1;
		if (debug)
			wcout << "Got: " << drname << endl;
	}

	if (debug) 
		wcout << L"Getting volume mountpoints (includes NTFS folders)" << endl
		<< L"Getting first volume" << endl;

	hVolume = FindFirstVolume(szVolumeName, MAX_PATH);
	if (hVolume == INVALID_HANDLE_VALUE)
		goto die;

	if (debug)
		wcout << L"Traversing through list of drives" << endl;

	while (GetLastError() != ERROR_NO_MORE_FILES) {
		if (debug)
			wcout << L"Path name for " << szVolumeName << L"= \"";
		volumeNameEnd = wcslen(szVolumeName) - 1;
		szVolumePathNames = reinterpret_cast<wchar_t*>(new WCHAR[dwVolumePathNamesLen]);

		while (!GetVolumePathNamesForVolumeName(szVolumeName, szVolumePathNames, dwVolumePathNamesLen, &dwVolumePathNamesLen)) {
			if (GetLastError() != ERROR_MORE_DATA)
				break;
			delete[] reinterpret_cast<wchar_t*>(szVolumePathNames);
			szVolumePathNames = reinterpret_cast<wchar_t*>(new WCHAR[dwVolumePathNamesLen]);

		}
		if (debug)
			wcout << szVolumePathNames << L"\"" << endl;

		//.insert() does the dublicate checking
		sDrives.insert(wstring(szVolumePathNames));
		FindNextVolume(hVolume, szVolumeName, MAX_PATH);
	}

	wcout << L"Creating vector from found volumes, removing cd drives etc.:" << endl;
	for (set<wstring>::iterator it = sDrives.begin(); it != sDrives.end(); ++it) {
		UINT type = GetDriveType(it->c_str());
		if (type == DRIVE_FIXED || type == DRIVE_REMOTE) {
			if (debug)
				wcout << L"\t" << *it << endl;
			vDrives.push_back(drive(*it));
		}
	}

	FindVolumeClose(hVolume);
	delete[] reinterpret_cast<wchar_t*>(szVolumePathNames);
	return -1;
 
die:
	if (hVolume)
		FindVolumeClose(hVolume);
	die();
	return 3;
}

int check_drives(vector<drive>& vDrives, printInfoStruct& printInfo) 
{
	wchar_t *slash = L"\\";

	if (debug)
		wcout << L"Parsing user input drive names" << endl;

	for (vector<wstring>::iterator it = printInfo.drives.begin();
			it != printInfo.drives.end(); ++it) {
		if (it->at(it->length() - 1) != *slash)
			it->append(slash);
		if (std::wstring::npos == it->find(L":\\")) {
			wcout << "A \":\" is required after the drive name of " << *it << endl;
			return 3;
		}
		if (debug)
			wcout << L"Added " << *it << endl;
		vDrives.push_back(drive(*it));
	}
	return -1;
}

bool getFreeAndCap(drive& drive, const Bunit& unit) 
{
	if (debug)
		wcout << L"Getting free disk space for drive " << drive.name << endl;
	ULARGE_INTEGER tempFree, tempTotal;
	if (!GetDiskFreeSpaceEx(drive.name.c_str(), NULL, &tempTotal, &tempFree)) {
		return FALSE;
	}
	if (debug)
		wcout << L"\tcap: " << tempFree.QuadPart << endl;
	drive.cap = round((tempTotal.QuadPart / pow(1024.0, unit)));
	if (debug)
		wcout << L"\tAfter converion: " << drive.cap << endl
		<< L"\tfree: " << tempFree.QuadPart << endl;
	drive.free = round((tempFree.QuadPart / pow(1024.0, unit)));
	if (debug)
		wcout << L"\tAfter converion: " << drive.free << endl << endl;

	return TRUE;
}