// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <shlwapi.h>
#include <winbase.h>

#define VERSION 1.0

namespace po = boost::program_options;

struct printInfoStruct
{
	threshold warn;
	threshold crit;
	double tRam;
	double aRam;
	double percentFree;
	Bunit unit = BunitMB;
	bool showUsed;
};

static bool l_Debug;

static int parseArguments(int ac, WCHAR ** av, po::variables_map& vm, printInfoStruct& printInfo)
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
		("show-used,U", "Show used memory instead of the free memory")
		;

	po::wcommand_line_parser parser(ac, av);

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
	} catch (const std::exception& e) {
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
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
		printInfo.warn.legal = !printInfo.warn.legal;
	}

	if (vm.count("critical")) {
		try {
			printInfo.crit = threshold(vm["critical"].as<std::wstring>());
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
		printInfo.crit.legal = !printInfo.crit.legal;
	}

	l_Debug = vm.count("debug") > 0;

	if (vm.count("unit")) {
		try {
			printInfo.unit = parseBUnit(vm["unit"].as<std::wstring>());
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}

	if (vm.count("show-used")) {
		printInfo.showUsed = true;
		printInfo.warn.legal = true;
		printInfo.crit.legal = true;
	}

	return -1;
}

static int printOutput(printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Constructing output string" << '\n';

	state state = OK;

	std::wcout << L"MEMORY ";

	double currentValue;

	if (!printInfo.showUsed)
		currentValue = printInfo.aRam;
	else
		currentValue = printInfo.tRam - printInfo.aRam;

	if (printInfo.warn.rend(currentValue, printInfo.tRam))
		state = WARNING;

	if (printInfo.crit.rend(currentValue, printInfo.tRam))
		state = CRITICAL;

	std::wcout << stateToString(state);

	if (!printInfo.showUsed)
		std::wcout << " - " << printInfo.percentFree << L"% free";
	else
		std::wcout << " - " << 100 - printInfo.percentFree << L"% used";

	std::wcout << "| 'memory'=" << currentValue << BunitStr(printInfo.unit) << L";"
		<< printInfo.warn.pString(printInfo.tRam) << L";" << printInfo.crit.pString(printInfo.tRam)
		<< L";0;" << printInfo.tRam << '\n';

	return state;
}

static int check_memory(printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Accessing memory statistics via MemoryStatus" << '\n';

	MEMORYSTATUSEX memBuf;
	memBuf.dwLength = sizeof(memBuf);
	GlobalMemoryStatusEx(&memBuf);

	printInfo.tRam = round((memBuf.ullTotalPhys / pow(1024.0, printInfo.unit) * pow(10.0, printInfo.unit))) / pow(10.0, printInfo.unit);
	printInfo.aRam = round((memBuf.ullAvailPhys / pow(1024.0, printInfo.unit) * pow(10.0, printInfo.unit))) / pow(10.0, printInfo.unit);
	printInfo.percentFree = 100.0 * memBuf.ullAvailPhys / memBuf.ullTotalPhys;

	if (l_Debug)
		std::wcout << L"Found memBuf.dwTotalPhys: " << memBuf.ullTotalPhys << '\n'
		<< L"Found memBuf.dwAvailPhys: " << memBuf.ullAvailPhys << '\n';

	return -1;
}

int wmain(int argc, WCHAR **argv)
{
	printInfoStruct printInfo = {};
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_memory(printInfo);
	if (ret != -1)
		return ret;

	return printOutput(printInfo);
}
