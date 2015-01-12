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

#include "thresholds.h"
#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <Pdh.h>
#include <Shlwapi.h>
#include <pdhmsg.h>
#include <iostream>

#define VERSION 1.0

namespace po = boost::program_options;

using std::endl; using std::cout; using std::wstring;
using std::wcout;

static BOOL debug = FALSE;

struct printInfoStruct 
{
	threshold warn, crit;
	double load;
};

static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(printInfoStruct&);
static int check_load(printInfoStruct&);

int wmain(int argc, wchar_t **argv) 
{
	printInfoStruct printInfo{ };
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_load(printInfo);
	if (ret != -1)
		return ret;

	return printOutput(printInfo);
}

int parseArguments(int ac, wchar_t **av, po::variables_map& vm, printInfoStruct& printInfo) {
	wchar_t namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	wchar_t *progName = PathFindFileName(namePath);

	po::options_description desc;

	desc.add_options()
		("help,h", "print usage message and exit")
		("version,V", "print version and exit")
		("debug,d", "Verbose/Debug output")
		("warning,w", po::wvalue<wstring>(), "warning value (in percent)")
		("critical,c", po::wvalue<wstring>(), "critical value (in percent)")
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
			L"%s is a simple program to check a machines CPU load.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tLOAD WARNING 67%%|load=67%%;50%%;90%%;0;100\n\n"
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
		cout << endl;
		return 0;
	}

	if (vm.count("version"))
		cout << "Version: " << VERSION << endl;

	if (vm.count("warning")) {
		try {
			std::wstring wthreshold = vm["warning"].as<wstring>();
			std::vector<std::wstring> tokens;
			boost::algorithm::split(tokens, wthreshold, boost::algorithm::is_any_of(","));
			printInfo.warn = threshold(tokens[0]);
		} catch (std::invalid_argument& e) {
			cout << e.what() << endl;
			return 3;
		}
	}
	if (vm.count("critical")) {
		try {
			std::wstring cthreshold = vm["critical"].as<wstring>();
			std::vector<std::wstring> tokens;
			boost::algorithm::split(tokens, cthreshold, boost::algorithm::is_any_of(","));
			printInfo.crit = threshold(tokens[0]);
		} catch (std::invalid_argument& e) {
			cout << e.what() << endl;
			return 3;
		}
	}

	if (vm.count("debug"))
		debug = TRUE;

	return -1;
}

int printOutput(printInfoStruct& printInfo) 
{
	if (debug)
		wcout << L"Constructing output string" << endl;

	state state = OK;

	if (printInfo.warn.rend(printInfo.load))
		state = WARNING;

	if (printInfo.crit.rend(printInfo.load))
		state = CRITICAL;

	std::wstringstream perf;
	perf << L"% | load=" << printInfo.load << L"%;" << printInfo.warn.pString() << L";" 
		<< printInfo.crit.pString() << L";0;100" << endl;

	switch (state) {
	case OK:
		wcout << L"LOAD OK " << printInfo.load << perf.str();
		break;
	case WARNING:
		wcout << L"LOAD WARNING " << printInfo.load << perf.str();
		break;
	case CRITICAL:
		wcout << L"LOAD CRITICAL " << printInfo.load << perf.str();
		break;
	}

	return state;
}

int check_load(printInfoStruct& printInfo) 
{
	PDH_HQUERY phQuery;
	PDH_HCOUNTER phCounter;
	DWORD dwBufferSize = 0;
	DWORD CounterType;
	PDH_FMT_COUNTERVALUE DisplayValue;
	PDH_STATUS err;

	LPCWSTR path = L"\\Processor(_Total)\\% Idle Time";

	if (debug)
		wcout << L"Creating query and adding counter" << endl;

	err = PdhOpenQuery(NULL, NULL, &phQuery);
	if (!SUCCEEDED(err))
		goto die;

	err = PdhAddEnglishCounter(phQuery, path, NULL, &phCounter);
	if (!SUCCEEDED(err))
		goto die;

	if (debug)
		wcout << L"Collecting first batch of query data" << endl;

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (debug)
		wcout << L"Sleep for one second" << endl;

	Sleep(1000);

	if (debug)
		wcout << L"Collecting second batch of query data" << endl;

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (debug)
		wcout << L"Creating formatted counter array" << endl;

	err = PdhGetFormattedCounterValue(phCounter, PDH_FMT_DOUBLE, &CounterType, &DisplayValue);
	if (SUCCEEDED(err)) {
		if (DisplayValue.CStatus == PDH_CSTATUS_VALID_DATA) {
			if (debug)
				wcout << L"Recieved Value of " << DisplayValue.doubleValue << L" (idle)" << endl;
			printInfo.load = 100.0 - DisplayValue.doubleValue;
		}

		if (debug)
			wcout << L"Finished collection. Cleaning up and returning" << endl;

		PdhCloseQuery(phQuery);
		return -1;
	}

die:
	die();
	if (phQuery)
		PdhCloseQuery(phQuery);
	return 3;
}