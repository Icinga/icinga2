// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <windows.h>
#include <shlwapi.h>

#define VERSION 1.1

namespace po = boost::program_options;

struct printInfoStruct
{
	bool warn;
	DWORD ServiceState;
	std::wstring service;
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
		("debug,D", "Verbose/Debug output")
		("service,s", po::wvalue<std::wstring>(), "Service name to check")
		("description,d", "Use \"service\" to match on description")
		("warn,w", "Return warning (1) instead of critical (2),\n when service is not running")
		;

	po::wcommand_line_parser parser(ac, av);

	try {
		po::store(
			parser
			.options(desc)
			.style(
			po::command_line_style::unix_style &
			~po::command_line_style::allow_guessing |
			po::command_line_style::allow_long_disguise
			)
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
			L"%s is a simple program to check the status of a service.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tSERVICE CRITICAL NOT_RUNNING | service=4;!4;!4;1;7\n\n"
			L"\"SERVICE\" being the type of the check, \"CRITICAL\" the returned status\n"
			L"and \"1\" is the returned value.\n"
			L"A service is either running (Code 0x04) or not running (any other).\n"
			L"For more information consult the msdn on service state transitions.\n\n"
			L"%s' exit codes denote the following:\n"
			L" 0\tOK,\n\tNo Thresholds were broken or the programs check part was not executed\n"
			L" 1\tWARNING,\n\tThe warning, but not the critical threshold was broken\n"
			L" 2\tCRITICAL,\n\tThe critical threshold was broken\n"
			L" 3\tUNKNOWN, \n\tThe program experienced an internal or input error\n\n"
			L"%s' thresholds work differently, since a service is either running or not\n"
			L"all \"-w\" and \"-c\" do is say whether a not running service is a warning\n"
			L"or critical state respectively.\n\n"
			, progName, progName);
		std::cout << '\n';
		return 0;
	}

	if (vm.count("version")) {
		std::cout << "Version: " << VERSION << '\n';
		return 0;
	}

	if (!vm.count("service")) {
		std::cout << "Argument \"service\" is required.\n" << desc << '\n';
		return 3;
	}

	printInfo.service = vm["service"].as<std::wstring>();

	printInfo.warn = vm.count("warn");

	l_Debug = vm.count("debug") > 0;

	return -1;
}

static int printOutput(const printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Constructing output string" << '\n';

	std::wstring perf;
	state state = OK;

	if (!printInfo.ServiceState) {
		std::wcout << L"SERVICE CRITICAL NOT FOUND | 'service'=" << printInfo.ServiceState << ";;;1;7" << '\n';
		return 3;
	}

	if (printInfo.ServiceState != 0x04)
		printInfo.warn ? state = WARNING : state = CRITICAL;

	switch (state) {
	case OK:
		std::wcout << L"SERVICE \"" << printInfo.service << "\" OK RUNNING | 'service'=4;;;1;7" << '\n';
		break;
	case WARNING:
		std::wcout << L"SERVICE \"" << printInfo.service << "\" WARNING NOT RUNNING | 'service'=" << printInfo.ServiceState << ";;;1;7" << '\n';
		break;
	case CRITICAL:
		std::wcout << L"SERVICE \"" << printInfo.service << "\" CRITICAL NOT RUNNING | 'service'=" << printInfo.ServiceState << ";;;1;7" << '\n';
		break;
	}

	return state;
}

static std::wstring getServiceByDescription(const std::wstring& description)
{
	SC_HANDLE hSCM = NULL;
	LPENUM_SERVICE_STATUSW lpServices = NULL;
	LPBYTE lpBuf = NULL;
	DWORD cbBufSize = 0;
	DWORD lpServicesReturned = 0;
	DWORD pcbBytesNeeded = 0;
	DWORD lpResumeHandle = 0;;

	if (l_Debug)
		std::wcout << L"Opening SC Manager" << '\n';

	hSCM = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (hSCM == NULL)
		goto die;

	if (l_Debug)
		std::wcout << L"Determining initially required memory" << '\n';

	EnumServicesStatus(hSCM, SERVICE_WIN32 | SERVICE_DRIVER, SERVICE_STATE_ALL, NULL, 0,
		&pcbBytesNeeded, &lpServicesReturned, &lpResumeHandle);

	/* This should always be ERROR_INSUFFICIENT_BUFFER... But for some reason it is sometimes ERROR_MORE_DATA
	* See the MSDN on EnumServiceStatus for a glimpse of despair
	*/

	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER && GetLastError() != ERROR_MORE_DATA)
		goto die;

	lpServices = reinterpret_cast<LPENUM_SERVICE_STATUSW>(new BYTE[pcbBytesNeeded]);

	if (l_Debug)
		std::wcout << L"Requesting Service Information. Entry point: " << lpResumeHandle << '\n';

	EnumServicesStatus(hSCM, SERVICE_WIN32 | SERVICE_DRIVER, SERVICE_STATE_ALL, lpServices, pcbBytesNeeded,
		&pcbBytesNeeded, &lpServicesReturned, &lpResumeHandle);

	for (decltype(lpServicesReturned) index = 0; index < lpServicesReturned; index++) {
		LPWSTR lpCurrent = lpServices[index].lpServiceName;

		if (l_Debug) {
			std::wcout << L"Opening Service \"" << lpServices[index].lpServiceName << L"\"\n";
		}

		SC_HANDLE hService = OpenService(hSCM, lpCurrent, SERVICE_QUERY_CONFIG);
		if (!hService)
			goto die;

		DWORD dwBytesNeeded = 0;
		if (l_Debug)
			std::wcout << "Accessing config\n";

		if (!QueryServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, NULL, 0, &dwBytesNeeded) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			continue;

		LPSERVICE_DESCRIPTION lpsd = reinterpret_cast<LPSERVICE_DESCRIPTION>(new BYTE[dwBytesNeeded]);

		if (!QueryServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, (LPBYTE)lpsd, dwBytesNeeded, &dwBytesNeeded))
			continue;

		if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, L"") != 0) {
			std::wstring desc(lpsd->lpDescription);
			if (l_Debug)
				std::wcout << "Got description:\n" << desc << '\n';
			size_t p = desc.find(description);
			if (desc.find(description) != desc.npos)
				return lpCurrent;
		}
		else if (l_Debug)
			std::wcout << "No description found\n";
	}

	CloseServiceHandle(hSCM);
	delete[] lpServices;
	return L"";

die:
	printErrorInfo();
	if (hSCM)
		CloseServiceHandle(hSCM);
	if (lpServices)
		delete[] lpServices;
	return L"";
}

static DWORD getServiceStatus(const printInfoStruct& printInfo)
{
	SC_HANDLE hSCM;
	SC_HANDLE hService;
	DWORD cbBufSize;
	DWORD lpResumeHandle = 0;
	LPBYTE lpBuf = NULL;

	if (l_Debug)
		std::wcout << L"Opening SC Manager" << '\n';

	hSCM = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (hSCM == NULL)
		goto die;

	hService = OpenService(hSCM, printInfo.service.c_str(), SERVICE_QUERY_STATUS);
	if (hService == NULL)
		goto die;

	QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, NULL, 0, &cbBufSize);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		goto die;

	lpBuf = new BYTE[cbBufSize];
	if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, lpBuf, cbBufSize, &cbBufSize)) {
		LPSERVICE_STATUS_PROCESS pInfo = (LPSERVICE_STATUS_PROCESS)lpBuf;
		return pInfo->dwCurrentState;
	}

die:
	printErrorInfo();
	if (hSCM)
		CloseServiceHandle(hSCM);
	if (hService)
		CloseServiceHandle(hService);
	if (lpBuf)
		delete [] lpBuf;

	return -1;
}

int wmain(int argc, WCHAR **argv)
{
	po::variables_map vm;
	printInfoStruct printInfo;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	if (vm.count("description"))
		printInfo.service = getServiceByDescription(vm["service"].as<std::wstring>());

	if (printInfo.service.empty()) {
		std::wcout << "Could not find service matching description\n";
		return 3;
	}

	printInfo.ServiceState = getServiceStatus(printInfo);
	if (printInfo.ServiceState == -1)
		return 3;

	return printOutput(printInfo);
}

