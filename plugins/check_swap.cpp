/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include <Pdh.h>
#include <iostream>

#include "thresholds.h"

#include "boost/program_options.hpp"

#define VERSION 1.0

namespace po = boost::program_options;

using std::endl; using std::wcout; using std::wstring;
using std::cout;

struct printInfoStruct 
{
	threshold warn, crit;
	double swap;
};

static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(printInfoStruct&);
static int check_swap(printInfoStruct&);

int wmain(int argc, wchar_t **argv) 
{
	printInfoStruct printInfo = { };
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_swap(printInfo);
	if (ret != -1)
		return ret;

	return printOutput(printInfo);
}

int parseArguments(int ac, wchar_t **av, po::variables_map& vm, printInfoStruct& printInfo) 
{
	wchar_t namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	wchar_t *progName = PathFindFileName(namePath);

	po::options_description desc;

	desc.add_options()
		(",h", "print help message and exit")
		("help", "print verbose help and exit")
		("version,v", "print version and exit")
		("warning,w", po::wvalue<wstring>(), "warning threshold")
		("critical,c", po::wvalue<wstring>(), "critical threshold")
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

	if (vm.count("h")) {
		cout << desc << endl;
		return 0;
	}
    
	if (vm.count("help")) {
		wcout << progName << " Help\n\tVersion: " << VERSION << endl;
		wprintf(
			L"%s is a simple program to check a machines swap in percent.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tSWAP WARNING 23.8304%%|swap=23.8304%%;19.5;30;0;100\n\n"
			L"\"SWAP\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"23.8304%%\" is the returned value.\n"
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
		cout << endl;
		return 0;
	}

	if (vm.count("version"))
		wcout << L"Version: " << VERSION << endl;

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

	return -1;
}

int printOutput(printInfoStruct& printInfo) 
{
	state state = OK;

	if (printInfo.warn.rend(printInfo.swap))
		state = WARNING;

	if (printInfo.crit.rend(printInfo.swap))
		state = CRITICAL;

	switch (state) {
	case OK:
		wcout << L"SWAP OK " << printInfo.swap << L"%|swap=" << printInfo.swap << L"%;" 
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;100" << endl;
		break;
	case WARNING:
		wcout << L"SWAP WARNING " << printInfo.swap << L"%|swap=" << printInfo.swap << L"%;"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;100" << endl;
		break;
	case CRITICAL:
		wcout << L"SWAP CRITICAL " << printInfo.swap << L"%|swap=" << printInfo.swap << L"%;"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;100" << endl;
		break;
	}

	return state;
}

int check_swap(printInfoStruct& printInfo) 
{
	PDH_HQUERY phQuery;
	PDH_HCOUNTER phCounter;
	DWORD dwBufferSize = 0;
	DWORD CounterType;
	PDH_FMT_COUNTERVALUE DisplayValue;
	PDH_STATUS err;

	LPCWSTR path = L"\\Paging File(*)\\% Usage";

	err = PdhOpenQuery(NULL, NULL, &phQuery);
	if (!SUCCEEDED(err))
		goto die;

	err = PdhAddEnglishCounter(phQuery, path, NULL, &phCounter);
	if (!SUCCEEDED(err))
		goto die;

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	err = PdhGetFormattedCounterValue(phCounter, PDH_FMT_DOUBLE, &CounterType, &DisplayValue);
	if (SUCCEEDED(err)) {
		printInfo.swap = DisplayValue.doubleValue;
		PdhCloseQuery(phQuery);
		return -1;
	}

die:
	if (phQuery)
		PdhCloseQuery(phQuery);
	die(err);
	return 3;
}