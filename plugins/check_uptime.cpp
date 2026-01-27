// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <boost/chrono.hpp>
#include <iostream>
#include <windows.h>
#include <shlwapi.h>

#define VERSION 1.0

namespace po = boost::program_options;

struct printInfoStruct
{
	threshold warn;
	threshold crit;
	long long time;
	long long timeInSeconds;
	Tunit unit;
};

static bool l_Debug;

static int parseArguments(int ac, WCHAR **av, po::variables_map& vm, printInfoStruct& printInfo)
{
	WCHAR namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	WCHAR *progName = PathFindFileName(namePath);

	po::options_description desc;

	desc.add_options()
		("help,h", "Print help message and exit")
		("version,V", "Print version and exit")
		("debug,d", "Verbose/Debug output")
		("warning,w", po::wvalue<std::wstring>(), "Warning threshold (Uses -unit)")
		("critical,c", po::wvalue<std::wstring>(), "Critical threshold (Uses -unit)")
		("unit,u", po::wvalue<std::wstring>(), "Unit to use:\nh\t- hours\nm\t- minutes\ns\t- seconds (default)\nms\t- milliseconds")
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
			L"%s is a simple program to check a machines uptime.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tUPTIME WARNING 712h | uptime=712h;700;1800;0\n\n"
			L"\"UPTIME\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"712h\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. Performance data will only be displayed when\n"
			L"you set at least one threshold\n\n"
			L"Note that the returned time ins always rounded down,\n"
			L"4 hours and 44 minutes will show as 4h.\n\n"
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

	if (vm.count("version")) {
		std::cout << VERSION << '\n';
		return 0;
	}

	if (vm.count("warning")) {
		try {
			printInfo.warn = threshold(vm["warning"].as<std::wstring>());
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}
	if (vm.count("critical")) {
		try {
			printInfo.crit = threshold(vm["critical"].as<std::wstring>());
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}

	if (vm.count("unit")) {
		try {
			printInfo.unit = parseTUnit(vm["unit"].as<std::wstring>());
		} catch (const std::invalid_argument&) {
			std::wcout << L"Unknown unit type " << vm["unit"].as<std::wstring>() << '\n';
			return 3;
		}
	} else
		printInfo.unit = TunitS;

	l_Debug = vm.count("debug") > 0;

	return -1;
}

static int printOutput(printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Constructing output string" << '\n';

	state state = OK;

	if (printInfo.warn.rend((double) printInfo.time))
		state = WARNING;
	if (printInfo.crit.rend((double) printInfo.time))
		state = CRITICAL;

	switch (state) {
	case OK:
		std::wcout << L"UPTIME OK " << printInfo.time << TunitStr(printInfo.unit) << L" | 'uptime'=" << printInfo.timeInSeconds
			<< "s" << L";" << printInfo.warn.toSeconds(printInfo.unit).pString() << L";"
			<< printInfo.crit.toSeconds(printInfo.unit).pString() << L";0;" << '\n';
		break;
	case WARNING:
		std::wcout << L"UPTIME WARNING " << printInfo.time << TunitStr(printInfo.unit) << L" | 'uptime'=" << printInfo.timeInSeconds
			<< "s" << L";" << printInfo.warn.toSeconds(printInfo.unit).pString() << L";"
			<< printInfo.crit.toSeconds(printInfo.unit).pString() << L";0;" << '\n';
		break;
	case CRITICAL:
		std::wcout << L"UPTIME CRITICAL " << printInfo.time << TunitStr(printInfo.unit) << L" | 'uptime'=" << printInfo.timeInSeconds
			<< "s" << L";" << printInfo.warn.toSeconds(printInfo.unit).pString() << L";"
			<< printInfo.crit.toSeconds(printInfo.unit).pString() << L";0;" << '\n';
		break;
	}

	return state;
}

static void getUptime(printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Getting uptime in milliseconds" << '\n';

	boost::chrono::milliseconds uptime = boost::chrono::milliseconds(GetTickCount64());

	if (l_Debug)
		std::wcout << L"Converting requested unit (default: seconds)" << '\n';

	switch (printInfo.unit) {
	case TunitH:
		printInfo.time = boost::chrono::duration_cast<boost::chrono::hours>(uptime).count();
		break;
	case TunitM:
		printInfo.time = boost::chrono::duration_cast<boost::chrono::minutes>(uptime).count();
		break;
	case TunitS:
		printInfo.time = boost::chrono::duration_cast<boost::chrono::seconds>(uptime).count();
		break;
	case TunitMS:
		printInfo.time = uptime.count();
		break;
	}

	// For the Performance Data we need the time in seconds
	printInfo.timeInSeconds = boost::chrono::duration_cast<boost::chrono::seconds>(uptime).count();
}

int wmain(int argc, WCHAR **argv)
{
	po::variables_map vm;
	printInfoStruct printInfo;
	int ret = parseArguments(argc, argv, vm, printInfo);

	if (ret != -1)
		return ret;

	getUptime(printInfo);

	return printOutput(printInfo);
}
