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
#include <Shlwapi.h>
#include <wtsapi32.h>
#include <iostream>

#include "thresholds.h"

#include "boost/program_options.hpp"

#define VERSION 1.0

namespace po = boost::program_options;

using std::endl; using std::wcout;
using std::cout; using std::wstring;

static BOOL debug = FALSE;

struct printInfoStruct 
{
	threshold warn, crit;
	double users;
};

static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(printInfoStruct&);
static int check_users(printInfoStruct&);

int wmain(int argc, wchar_t **argv) 
{
	printInfoStruct printInfo = { };
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_users(printInfo);
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
		("help,h", "print help message and exit")
		("version,V", "print version and exit")
		("debug,d", "Verbose/Debug output")
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

	if (vm.count("help")) {
		wcout << progName << " Help\n\tVersion: " << VERSION << endl;
		wprintf(
			L"%s is a simple program to check a machines logged in users.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tUSERS WARNING 48 | users=48;10;50;0\n\n"
			L"\"USERS\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"48\" is the returned value.\n"
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
			L"All of these options work with the critical threshold \"-c\" too."
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

	if (vm.count("debug"))
		debug = TRUE;

	return -1;
}

int printOutput(printInfoStruct& printInfo) 
{
	if (debug)
		wcout << L"Constructing output string" << endl;

	state state = OK;

	if (printInfo.warn.rend(printInfo.users))
		state = WARNING;

	if (printInfo.crit.rend(printInfo.users))
		state = CRITICAL;

	switch (state) {
	case OK:
		wcout << L"USERS OK " << printInfo.users << L" User(s) logged in | users=" << printInfo.users << L";"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;" << endl;
		break;
	case WARNING:
		wcout << L"USERS WARNING " << printInfo.users << L" User(s) logged in | users=" << printInfo.users << L";"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;" << endl;
		break;
	case CRITICAL:
		wcout << L"USERS CRITICAL " << printInfo.users << L" User(s) logged in | users=" << printInfo.users << L";"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;" << endl;
		break;
	}

	return state;
}

int check_users(printInfoStruct& printInfo) 
{
	double users = 0;
	WTS_SESSION_INFOW *pSessionInfo = NULL;
	DWORD count;
	DWORD index;

	if (debug) 
		wcout << L"Trying to enumerate terminal sessions" << endl;
	
	if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &count)) {
		wcout << L"Failed to enumerate terminal sessions" << endl;
		die();
		if (pSessionInfo)
			WTSFreeMemory(pSessionInfo);
		return 3;
	}

	if (debug)
		wcout << L"Got all sessions (" << count << L"), traversing and counting active ones" << endl;

	for (index = 0; index < count; index++) {
		LPWSTR name;
		DWORD size;
		int len;

		if (debug)
			wcout << L"Querrying session number " << index << endl;

		if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, pSessionInfo[index].SessionId,
										WTSUserName, &name, &size))
			continue;

		if (debug)
			wcout << L"Found \"" << name << L"\". Checking whether it's a real session" << endl;

		len = lstrlenW(name);

		WTSFreeMemory(name);

		if (!len)
			continue;
		
		if (pSessionInfo[index].State == WTSActive || pSessionInfo[index].State == WTSDisconnected) {
			users++;
			if (debug)
				wcout << L"\"" << name << L"\" is a real session, counting it. Now " << users << endl;
		}
	}

	if (debug)
		wcout << "Finished coutning user sessions (" << users << "). Freeing memory and returning" << endl;

	WTSFreeMemory(pSessionInfo);
	printInfo.users = users;
	return -1;
}