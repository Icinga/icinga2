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
#include <Windows.h>
#include <Shlwapi.h>
#include <iostream>

#include "thresholds.h"

#include "boost/chrono.hpp"
#include "boost/program_options.hpp"

#define VERSION 1.0

namespace po = boost::program_options;

using std::cout; using std::endl;
using std::wcout; using std::wstring;

static BOOL debug;

struct printInfoStruct 
{
	threshold warn, crit;
	long long time;
	Tunit unit;
};


static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(printInfoStruct&);
static void getUptime(printInfoStruct&);

int wmain(int argc, wchar_t **argv)
{
	po::variables_map vm;
	printInfoStruct printInfo = { };
	int ret = parseArguments(argc, argv, vm, printInfo);
	
	if (ret != -1)
		return ret;

	getUptime(printInfo);

	return printOutput(printInfo);
}

int parseArguments(int ac, wchar_t **av, po::variables_map& vm, printInfoStruct& printInfo) 
{
	wchar_t namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	wchar_t *progName = PathFindFileName(namePath);

	po::options_description desc;
	
	desc.add_options()
		("help,h", "print help message and exit")
		("version,V", "print version and exit")
		("warning,w", po::wvalue<wstring>(), "warning threshold (Uses -unit)")
		("debug,d", "Verbose/Debug output")
		("critical,c", po::wvalue<wstring>(), "critical threshold (Uses -unit)")
		("unit,u", po::wvalue<wstring>(), "desired unit of output\nh\t- hours\nm\t- minutes\ns\t- seconds (default)\nms\t- milliseconds")
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
			L"%s is a simple program to check a machines uptime.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tUPTIME WARNING 712h|uptime=712h;700;1800;0\n\n"
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
			cout << endl;
		return 0;
	}

	if (vm.count("version")) {
		cout << VERSION << endl;
		return 0;
	}

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

	if (vm.count("unit")) {
		try{
			printInfo.unit = parseTUnit(vm["unit"].as<wstring>());
		} catch (std::invalid_argument) {

		} wcout << L"Unknown unit type " << vm["unit"].as<wstring>() << endl;
	} else
		printInfo.unit = TunitS;

	if (vm.count("debug"))
		debug = TRUE;
    
	return -1;
}

static int printOutput(printInfoStruct& printInfo) 
{
	if (debug)
		wcout << L"Constructing output string" << endl;

	state state = OK;

	if (printInfo.warn.rend(printInfo.time))
		state = WARNING;
	if (printInfo.crit.rend(printInfo.time))
		state = CRITICAL;

	switch (state) {
	case OK:
		wcout << L"UPTIME OK " << printInfo.time << TunitStr(printInfo.unit) << L" | uptime=" << printInfo.time
			<< TunitStr(printInfo.unit) << L";" << printInfo.warn.pString() << L";"
			<< printInfo.crit.pString() << L";0;" << endl;
		break;
	case WARNING:
		wcout << L"UPTIME WARNING " << printInfo.time << TunitStr(printInfo.unit) << L" | uptime=" << printInfo.time
			<< TunitStr(printInfo.unit) << L";" << printInfo.warn.pString() << L";"
			<< printInfo.crit.pString() << L";0;" << endl;
		break;
	case CRITICAL:
		wcout << L"UPTIME CRITICAL " << printInfo.time << TunitStr(printInfo.unit) << L" | uptime=" << printInfo.time
			<< TunitStr(printInfo.unit) << L";" << printInfo.warn.pString() << L";"
			<< printInfo.crit.pString() << L";0;" << endl;
		break;
	}

	return state;
}

void getUptime(printInfoStruct& printInfo) 
{
	if (debug)
		wcout << L"Getting uptime in milliseconds" << endl;

	boost::chrono::milliseconds uptime = boost::chrono::milliseconds(GetTickCount64());
	
	if (debug)
		wcout << L"Converting requested unit (default: seconds)" << endl;

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
}