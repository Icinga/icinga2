// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <windows.h>
#include <shlwapi.h>
#include <wtsapi32.h>

#define VERSION 1.0

namespace po = boost::program_options;

struct printInfoStruct
{
	threshold warn;
	threshold crit;
	DOUBLE users;
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
		("warning,w", po::wvalue<std::wstring>(), "Warning threshold")
		("critical,c", po::wvalue<std::wstring>(), "Critical threshold")
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
			L"%s is a simple program to check a machines logged in users.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
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
	}
	if (vm.count("critical")) {
		try {
			printInfo.crit = threshold(vm["critical"].as<std::wstring>());
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

	if (printInfo.warn.rend(printInfo.users))
		state = WARNING;

	if (printInfo.crit.rend(printInfo.users))
		state = CRITICAL;

	switch (state) {
	case OK:
		std::wcout << L"USERS OK " << printInfo.users << L" User(s) logged in | 'users'=" << printInfo.users << L";"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;" << '\n';
		break;
	case WARNING:
		std::wcout << L"USERS WARNING " << printInfo.users << L" User(s) logged in | 'users'=" << printInfo.users << L";"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;" << '\n';
		break;
	case CRITICAL:
		std::wcout << L"USERS CRITICAL " << printInfo.users << L" User(s) logged in | 'users'=" << printInfo.users << L";"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;" << '\n';
		break;
	}

	return state;
}

static int check_users(printInfoStruct& printInfo)
{
	DOUBLE users = 0;
	WTS_SESSION_INFOW *pSessionInfo = NULL;
	DWORD count;
	DWORD index;

	if (l_Debug)
		std::wcout << L"Trying to enumerate terminal sessions" << '\n';

	if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &count)) {
		std::wcout << L"Failed to enumerate terminal sessions" << '\n';
		printErrorInfo();
		if (pSessionInfo)
			WTSFreeMemory(pSessionInfo);
		return 3;
	}

	if (l_Debug)
		std::wcout << L"Got all sessions (" << count << L"), traversing and counting active ones" << '\n';

	for (index = 0; index < count; index++) {
		LPWSTR name;
		DWORD size;
		int len;

		if (l_Debug)
			std::wcout << L"Querrying session number " << index << '\n';

		if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, pSessionInfo[index].SessionId,
			WTSUserName, &name, &size))
			continue;

		if (l_Debug)
			std::wcout << L"Found \"" << name << L"\". Checking whether it's a real session" << '\n';

		len = lstrlenW(name);

		WTSFreeMemory(name);

		if (!len)
			continue;

		if (pSessionInfo[index].State == WTSActive || pSessionInfo[index].State == WTSDisconnected) {
			users++;
			if (l_Debug)
				std::wcout << L"\"" << name << L"\" is a real session, counting it. Now " << users << '\n';
		}
	}

	if (l_Debug)
		std::wcout << "Finished coutning user sessions (" << users << "). Freeing memory and returning" << '\n';

	WTSFreeMemory(pSessionInfo);
	printInfo.users = users;
	return -1;
}

int wmain(int argc, WCHAR **argv)
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
