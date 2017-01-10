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

#include <Pdh.h>
#include <Shlwapi.h>
#include <pdhmsg.h>
#include <iostream>

#include "check_load.h"

#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"

#define VERSION 1.0

namespace po = boost::program_options;

static BOOL debug = FALSE;


INT wmain(INT argc, WCHAR **argv) 
{
	printInfoStruct printInfo{ };
	po::variables_map vm;

	INT ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_load(printInfo);
	if (ret != -1)
		return ret;

	return printOutput(printInfo);
}

INT parseArguments(INT ac, WCHAR **av, po::variables_map& vm, printInfoStruct& printInfo) {
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
		} catch (std::invalid_argument& e) {
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
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}

	if (vm.count("debug"))
		debug = TRUE;

	return -1;
}

INT printOutput(printInfoStruct& printInfo) 
{
	if (debug)
		std::wcout << L"Constructing output string" << '\n';

	state state = OK;

	if (printInfo.warn.rend(printInfo.load))
		state = WARNING;

	if (printInfo.crit.rend(printInfo.load))
		state = CRITICAL;

	std::wstringstream perf;
	perf << L"% | load=" << printInfo.load << L"%;" << printInfo.warn.pString() << L";" 
		<< printInfo.crit.pString() << L";0;100" << '\n';

	switch (state) {
	case OK:
		std::wcout << L"LOAD OK " << printInfo.load << perf.str();
		break;
	case WARNING:
		std::wcout << L"LOAD WARNING " << printInfo.load << perf.str();
		break;
	case CRITICAL:
		std::wcout << L"LOAD CRITICAL " << printInfo.load << perf.str();
		break;
	}

	return state;
}

INT check_load(printInfoStruct& printInfo) 
{
	PDH_HQUERY phQuery = NULL;
	PDH_HCOUNTER phCounter;
	DWORD dwBufferSize = 0;
	DWORD CounterType;
	PDH_FMT_COUNTERVALUE DisplayValue;
	PDH_STATUS err;

	LPCWSTR path = L"\\Processor(_Total)\\% Idle Time";

	if (debug)
		std::wcout << L"Creating query and adding counter" << '\n';

	err = PdhOpenQuery(NULL, NULL, &phQuery);
	if (!SUCCEEDED(err))
		goto die;

	err = PdhAddEnglishCounter(phQuery, path, NULL, &phCounter);
	if (!SUCCEEDED(err))
		goto die;

	if (debug)
		std::wcout << L"Collecting first batch of query data" << '\n';

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (debug)
		std::wcout << L"Sleep for one second" << '\n';

	Sleep(1000);

	if (debug)
		std::wcout << L"Collecting second batch of query data" << '\n';

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (debug)
		std::wcout << L"Creating formatted counter array" << '\n';

	err = PdhGetFormattedCounterValue(phCounter, PDH_FMT_DOUBLE, &CounterType, &DisplayValue);
	if (SUCCEEDED(err)) {
		if (DisplayValue.CStatus == PDH_CSTATUS_VALID_DATA) {
			if (debug)
				std::wcout << L"Recieved Value of " << DisplayValue.doubleValue << L" (idle)" << '\n';
			printInfo.load = 100.0 - DisplayValue.doubleValue;
		} else {
			if (debug)
				std::wcout << L"Received data was not valid\n";
			goto die;
		}

		if (debug)
			std::wcout << L"Finished collection. Cleaning up and returning" << '\n';

		PdhCloseQuery(phQuery);
		return -1;
	}

die:
	die();
	if (phQuery)
		PdhCloseQuery(phQuery);
	return 3;
}
