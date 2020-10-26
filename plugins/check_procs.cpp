/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <windows.h>
#include <shlwapi.h>
#include <tlhelp32.h>

#define VERSION 1.1

namespace po = boost::program_options;

struct printInfoStruct
{
	threshold warn;
	threshold crit;
	std::wstring command;
	std::wstring user;
};

struct ProcSnapshotResources {
	HANDLE hProcessSnap{ nullptr }, hProcess{ nullptr };
	PROCESSENTRY32 pe32{};

	~ProcSnapshotResources()
	{
		if (hProcess)
			CloseHandle(hProcess);
		if (hProcessSnap)
			CloseHandle(hProcessSnap);
	}
};

struct ProcTokenResources {
	HANDLE hToken{ nullptr };
	PTOKEN_USER pSIDTokenUser{ nullptr };
	SID_NAME_USE sidNameUse{ SidTypeUnknown };
	LPWSTR AcctName{ nullptr }, DomainName{ nullptr };
	DWORD dwReturnLength{ 1 }, dwAcctName{ 1 }, dwDomainName{ 1 };

	~ProcTokenResources()
	{
		delete[] reinterpret_cast<LPWSTR>(AcctName);
		delete[] reinterpret_cast<LPWSTR>(DomainName);
		if (pSIDTokenUser)
			delete[] reinterpret_cast<PTOKEN_USER>(pSIDTokenUser);
		if (hToken)
			CloseHandle(hToken);
	}
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
		("command,C", po::wvalue<std::wstring>(), "Only scan for matches of COMMAND (without path)")
		("user,u", po::wvalue<std::wstring>(), "Count only processes of user")
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
			L"%s is a simple program to check a machines processes.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tPROCS WARNING 67 | load=67;50;90;0\n\n"
			L"\"PROCS\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"67\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. Performance data will only be displayed when\n"
			L"you set at least one threshold\n\n"
			L"For \"-command\" and \"-user\" options keep in mind you need root to see other\n"
			L"users processes.\n\n"
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

	if (vm.count("version")) {
		std::wcout << "Version: " << VERSION << '\n';
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

	if (vm.count("command"))
		printInfo.command = vm["command"].as<std::wstring>();

	if (vm.count("user"))
		printInfo.user = vm["user"].as<std::wstring>();

	l_Debug = vm.count("debug") > 0;

	return -1;
}

static int printOutput(const int numProcs, printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Constructing output string" << '\n';

	state state = OK;

	if (printInfo.warn.rend(numProcs))
		state = WARNING;

	if (printInfo.crit.rend(numProcs))
		state = CRITICAL;

	std::wstring user;
	if (!printInfo.user.empty())
		user = L" processes of user " + printInfo.user;

	std::wcout << L"PROCS ";

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

	std::wcout << L" " << numProcs << user << L" | procs=" << numProcs << L";"
		<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0;" << '\n';

	return state;
}

static bool matchProcessImageName(HANDLE hProcess, const std::wstring& command)
{
	WCHAR wpath[MAX_PATH];
	DWORD dwFileSize = MAX_PATH;

	if (QueryFullProcessImageName(hProcess, 0, wpath, &dwFileSize)) {
		const WCHAR* w_command = command.c_str();
		const WCHAR* w_processImage = PathFindFileName(wpath);

		if (l_Debug)
			std::wcout << L"Comparing process image " << wpath << L" (" << w_processImage << L") to " << command << '\n';

		if (w_processImage != nullptr && wcsstr(w_processImage, w_command) != nullptr)
			return true;
	}
	return false;
}

static bool matchProcessUser(HANDLE hProcess, const std::wstring& user)
{
	ProcTokenResources my{};
	const WCHAR* wuser = user.c_str();

	//get ProcessToken
	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &my.hToken)) {
		//Won't count pid 0 (system idle) and 4/8 (Sytem)
		if (l_Debug)
			std::wcout << L"OpenProcessToken failed" << '\n';
		return false;
	}

	//Get dwReturnLength in first call
	if (!GetTokenInformation(my.hToken, TokenUser, nullptr, 0, &my.dwReturnLength)
		&& GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return false;

	my.pSIDTokenUser = reinterpret_cast<PTOKEN_USER>(new BYTE[my.dwReturnLength]);
	memset(my.pSIDTokenUser, 0, my.dwReturnLength);

	if (l_Debug)
		std::wcout << L"Received token, saving information" << '\n';

	//write Info in pSIDTokenUser
	if (!GetTokenInformation(my.hToken, TokenUser, my.pSIDTokenUser, my.dwReturnLength, &my.dwReturnLength))
		return false;

	if (l_Debug)
		std::wcout << L"Looking up SID" << '\n';

	//get dwAcctName and dwDomainName size
	if (!LookupAccountSid(NULL, my.pSIDTokenUser->User.Sid, my.AcctName,
		(LPDWORD)&my.dwAcctName, my.DomainName, (LPDWORD)&my.dwDomainName, &my.sidNameUse)
		&& GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return false;

	my.AcctName = reinterpret_cast<LPWSTR>(new WCHAR[my.dwAcctName]);
	my.DomainName = reinterpret_cast<LPWSTR>(new WCHAR[my.dwDomainName]);

	if (!LookupAccountSid(nullptr, my.pSIDTokenUser->User.Sid, my.AcctName,
		(LPDWORD)&my.dwAcctName, my.DomainName, (LPDWORD)&my.dwDomainName, &my.sidNameUse))
		return false;

	if (l_Debug)
		std::wcout << L"Comparing " << my.AcctName << L" to " << wuser << '\n';

	if (wcscmp(my.AcctName, wuser))
		return false;

	return true;
}

static int countProcs(const std::wstring& command, const std::wstring& user)
{
	ProcSnapshotResources my{};
	int numProcs = 0;
	const bool evaluateDetails = !command.empty() || !user.empty();

	if (l_Debug) {
		std::wcout << L"Counting all processes";
		if (!command.empty()) {
			std::wcout << L" with name " << command;
		}
		if (!user.empty()) {
			std::wcout << L" of user " << user;
		}
		std::wcout << '\n';
	}

	if (l_Debug)
		std::wcout << L"Creating snapshot" << '\n';

	my.hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (my.hProcessSnap == INVALID_HANDLE_VALUE)
		return 0;

	my.pe32.dwSize = sizeof(PROCESSENTRY32);

	if (l_Debug)
		std::wcout << L"Grabbing first proccess" << '\n';

	if (!Process32First(my.hProcessSnap, &my.pe32))
		return 0;

	if (l_Debug)
		std::wcout << L"Counting processes..." << '\n';

	do {
		bool processMatches = true;
		if (evaluateDetails) {
			if (l_Debug)
				std::wcout << L"Query process information" << '\n';

			my.hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, my.pe32.th32ProcessID);
			if (!command.empty() && !matchProcessImageName(my.hProcess, command)) {
				processMatches = false;
			} else {
				if (l_Debug)
					std::wcout << L"  Is command " << command << '\n';
			}

			if (!user.empty() && !matchProcessUser(my.hProcess, user)) {
				processMatches = false;
			} else {
				if (l_Debug)
					std::wcout << L"  Is process of " << user << '\n';
			}

			if (processMatches) {
				++numProcs;
				if (l_Debug)
					std::wcout << L"  (" << numProcs << L")" << '\n';
			}
		}
	} while (Process32Next(my.hProcessSnap, &my.pe32));

	return numProcs;
}

int wmain(int argc, WCHAR **argv)
{
	po::variables_map vm;
	printInfoStruct printInfo = { };

	int r = parseArguments(argc, argv, vm, printInfo);

	if (r != -1)
		return r;

	return printOutput(countProcs(printInfo.command, printInfo.user), printInfo);
}
