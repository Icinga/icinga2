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
#include <Shlwapi.h>
#include <iostream>
#include <WinBase.h>

#include "check_memory.h"

#define VERSION 1.0

namespace po = boost::program_options;

static BOOL debug = FALSE;

INT wmain(INT argc, WCHAR **argv)
{
	printInfoStruct printInfo = {};
	po::variables_map vm;

	INT ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_memory(printInfo);
	if (ret != -1)
		return ret;

	return printOutput(printInfo);
}

INT parseArguments(INT ac, WCHAR ** av, po::variables_map& vm, printInfoStruct& printInfo)
{
	WCHAR namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	WCHAR *progName = PathFindFileName(namePath);

	po::options_description desc;

	desc.add_options()
		("help,h", "Print help message and exit")
		("version,V", "Print version and exit")
		("debug,d", "Verbose/Debug output")
		("warning,w", po::wvalue<std::wstring>(), "Warning threshold")
		("critical,c", po::wvalue<std::wstring>(), "Critical threshold")
		("unit,u", po::wvalue<std::wstring>(), "The unit to use for display (default MB)")
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
			L"%s is a simple program to check a machines physical memory.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tMEMORY WARNING - 50%% free | memory=2024MB;3000;500;0;4096\n\n"
			L"\"MEMORY\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"50%%\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. Performance data will only be displayed when\n"
			L"you set at least one threshold\n\n"
			L"%s' exit codes denote the following:\n"
			L" 0\tOK,\n\tNo Thresholds were broken or the programs check part was not executed\n"
			L" 1\tWARNING,\n\tThe warning, but not the critical threshold was broken\n"
			L" 2\tCRITICAL,\n\tThe critical threshold was broken\n"
			L" 3\tUNKNOWN, \n\tThe program experienced an internal or input error\n\n"
			L"Threshold syntax:\n\n"
			L"-w THRESHOLD\n"
			L"warn if threshold is broken, which means VALUE > THRESHOLD\n"
			L"(unless stated differently)\n\n"
			L"-w !THRESHOLD\n"
			L"inverts threshold check, VALUE < THRESHOLD (analogous to above)\n\n"
			L"-w [THR1-THR2]\n"
			L"warn is VALUE is inside the range spanned by THR1 and THR2\n\n"
			L"-w ![THR1-THR2]\n"
			L"warn if VALUE is outside the range spanned by THR1 and THR2\n\n"
			L"-w THRESHOLD%%\n"
			L"if the plugin accepts percentage based thresholds those will be used.\n"
			L"Does nothing if the plugin does not accept percentages, or only uses\n"
			L"percentage thresholds. Ranges can be used with \"%%\", but both range values need\n"
			L"to end with a percentage sign.\n\n"
			L"All of these options work with the critical threshold \"-c\" too.\n"
			, progName);
		std::cout << '\n';
		return 0;
	}

	if (vm.count("version"))
		std::wcout << L"Version: " << VERSION << '\n';

	if (vm.count("warning")) {
		try {
			printInfo.warn = threshold(vm["warning"].as<std::wstring>());
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
		printInfo.warn.legal = !printInfo.warn.legal;
	}

	if (vm.count("critical")) {
		try {
			printInfo.crit = threshold(vm["critical"].as<std::wstring>());
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
		printInfo.crit.legal = !printInfo.crit.legal;
	}

	if (vm.count("debug"))
		debug = TRUE;

	if (vm.count("unit")) {
		try {
			printInfo.unit = parseBUnit(vm["unit"].as<std::wstring>());
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}

	return -1;
}

INT printOutput(printInfoStruct& printInfo)
{
	if (debug)
		std::wcout << L"Constructing output string" << '\n';

	state state = OK;

	if (printInfo.warn.rend(printInfo.aRam, printInfo.tRam))
		state = WARNING;

	if (printInfo.crit.rend(printInfo.aRam, printInfo.tRam))
		state = CRITICAL;

	switch (state) {
	case OK:
		std::wcout << L"MEMORY OK - " << printInfo.percentFree << L"% free | memory=" << printInfo.aRam << BunitStr(printInfo.unit) << L";"
			<< printInfo.warn.pString(printInfo.tRam) << L";" << printInfo.crit.pString(printInfo.tRam)
			<< L";0;" << printInfo.tRam << '\n';
		break;
	case WARNING:
		std::wcout << L"MEMORY WARNING - " << printInfo.percentFree << L"% free | memory=" << printInfo.aRam << BunitStr(printInfo.unit) << L";"
			<< printInfo.warn.pString(printInfo.tRam) << L";" << printInfo.crit.pString(printInfo.tRam)
			<< L";0;" << printInfo.tRam << '\n';
		break;
	case CRITICAL:
		std::wcout << L"MEMORY CRITICAL - " << printInfo.percentFree << L"% free | memory=" << printInfo.aRam << BunitStr(printInfo.unit) << L";"
			<< printInfo.warn.pString(printInfo.tRam) << L";" << printInfo.crit.pString(printInfo.tRam)
			<< L";0;" << printInfo.tRam << '\n';
		break;
	}

	return state;
}

INT check_memory(printInfoStruct& printInfo)
{
	if (debug)
		std::wcout << L"Accessing memory statistics via MemoryStatus" << '\n';

	_MEMORYSTATUSEX *pMemBuf = new _MEMORYSTATUSEX;

	pMemBuf->dwLength = sizeof(*pMemBuf);

	GlobalMemoryStatusEx(pMemBuf);

	printInfo.tRam = round(pMemBuf->ullTotalPhys / pow(1024.0, printInfo.unit));
	printInfo.aRam = round(pMemBuf->ullAvailPhys / pow(1024.0, printInfo.unit));
	printInfo.percentFree = 100.0 * pMemBuf->ullAvailPhys / pMemBuf->ullTotalPhys;

	if (debug)
		std::wcout << L"Found pMemBuf->dwTotalPhys: " << pMemBuf->ullTotalPhys << '\n'
		<< L"Found pMemBuf->dwAvailPhys: " << pMemBuf->ullAvailPhys << '\n';

	delete pMemBuf;

	return -1;
}
