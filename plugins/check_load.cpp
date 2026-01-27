// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <iostream>
#include <pdh.h>
#include <shlwapi.h>
#include <pdhmsg.h>

#define VERSION 1.0

namespace po = boost::program_options;

struct printInfoStruct
{
	threshold warn;
	threshold crit;
	double load;
};

static bool l_Debug;

static int parseArguments(int ac, WCHAR **av, po::variables_map& vm, printInfoStruct& printInfo)
{
	wchar_t namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	wchar_t *progName = PathFindFileName(namePath);

	po::options_description desc;

	desc.add_options()
		("help,h", "Print usage message and exit")
		("version,V", "Print version and exit")
		("debug,d", "Verbose/Debug output")
		("warning,w", po::wvalue<std::wstring>(), "Warning value (in percent)")
		("critical,c", po::wvalue<std::wstring>(), "Critical value (in percent)")
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
			L"%s is a simple program to check a machines CPU load.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tLOAD WARNING 67%% | load=67%%;50%%;90%%;0;100\n\n"
			L"\"LOAD\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"67%%\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value.\n\n"
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
			L"All of these options work with the critical threshold \"-c\" too."
			, progName);
		std::cout << '\n';
		return 0;
	}

	if (vm.count("version"))
		std::cout << "Version: " << VERSION << '\n';

	if (vm.count("warning")) {
		try {
			std::wstring wthreshold = vm["warning"].as<std::wstring>();
			std::vector<std::wstring> tokens;
			boost::algorithm::split(tokens, wthreshold, boost::algorithm::is_any_of(","));
			printInfo.warn = threshold(tokens[0]);
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}

	if (vm.count("critical")) {
		try {
			std::wstring cthreshold = vm["critical"].as<std::wstring>();
			std::vector<std::wstring> tokens;
			boost::algorithm::split(tokens, cthreshold, boost::algorithm::is_any_of(","));
			printInfo.crit = threshold(tokens[0]);
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}

	l_Debug = vm.count("debug") > 0;

	return -1;
}

static int printOutput(printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Constructing output string" << '\n';

	state state = OK;

	if (printInfo.warn.rend(printInfo.load))
		state = WARNING;

	if (printInfo.crit.rend(printInfo.load))
		state = CRITICAL;

	std::wcout << L"LOAD ";

	switch (state) {
	case OK:
		std::wcout << L"OK";
		break;
	case WARNING:
		std::wcout << L"WARNING";
		break;
	case CRITICAL:
		std::wcout << L"CRITICAL";
		break;
	}

	std::wcout << " " << printInfo.load << L"% | 'load'=" << printInfo.load << L"%;"
		<< printInfo.warn.pString() << L";"
		<< printInfo.crit.pString() << L";0;100" << '\n';

	return state;
}

static int check_load(printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Creating query and adding counter" << '\n';

	PDH_HQUERY phQuery;
	PDH_STATUS err = PdhOpenQuery(NULL, NULL, &phQuery);
	if (!SUCCEEDED(err))
		goto die;

	PDH_HCOUNTER phCounter;
	err = PdhAddEnglishCounter(phQuery, L"\\Processor(_Total)\\% Idle Time", NULL, &phCounter);
	if (!SUCCEEDED(err))
		goto die;

	if (l_Debug)
		std::wcout << L"Collecting first batch of query data" << '\n';

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (l_Debug)
		std::wcout << L"Sleep for one second" << '\n';

	Sleep(1000);

	if (l_Debug)
		std::wcout << L"Collecting second batch of query data" << '\n';

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (l_Debug)
		std::wcout << L"Creating formatted counter array" << '\n';

	DWORD CounterType;
	PDH_FMT_COUNTERVALUE DisplayValue;
	err = PdhGetFormattedCounterValue(phCounter, PDH_FMT_DOUBLE, &CounterType, &DisplayValue);
	if (SUCCEEDED(err)) {
		if (DisplayValue.CStatus == PDH_CSTATUS_VALID_DATA) {
			if (l_Debug)
				std::wcout << L"Recieved Value of " << DisplayValue.doubleValue << L" (idle)" << '\n';
			printInfo.load = 100.0 - DisplayValue.doubleValue;
		}
		else {
			if (l_Debug)
				std::wcout << L"Received data was not valid\n";
			goto die;
		}

		if (l_Debug)
			std::wcout << L"Finished collection. Cleaning up and returning" << '\n';

		PdhCloseQuery(phQuery);
		return -1;
	}

die:
	printErrorInfo();
	if (phQuery)
		PdhCloseQuery(phQuery);
	return 3;
}

int wmain(int argc, WCHAR **argv)
{
	printInfoStruct printInfo;
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_load(printInfo);
	if (ret != -1)
		return ret;

	return printOutput(printInfo);
}
