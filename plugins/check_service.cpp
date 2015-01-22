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

#include "boost/program_options.hpp"

#define VERSION 1.0

namespace po = boost::program_options;

using std::wcout; using std::endl;
using std::cout; using std::wstring;

static BOOL debug;

struct printInfoStruct 
{
	bool warn;
	int ServiceState;
	wstring service;
};

static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(const printInfoStruct&);
static int ServiceStatus(const printInfoStruct&);

int wmain(int argc, wchar_t **argv)
{
	po::variables_map vm;
	printInfoStruct printInfo = { false, 0, L"" };

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	printInfo.ServiceState = ServiceStatus(printInfo);
	if (printInfo.ServiceState == -1)
		return 3;

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
		("service,s", po::wvalue<wstring>(), "service to check (required)")
		("warn,w", "return warning (1) instead of critical (2),\n when service is not running")
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
			L"%s is a simple program to check the status of a service.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tSERVICE CRITICAL NOT_RUNNING|service=4;!4;!4;1;7\n\n"
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
			L"Known issue: Since icinga2 runs as NETWORK SERVICE it can't access the access control lists\n"
			L"it will not be able to find a service like NTDS. To fix this add ACL read permissions to icinga2.\n"
			, progName, progName);
		cout << endl;
		return 0;
	}
    
	 if (vm.count("version")) {
		cout << "Version: " << VERSION << endl;
		return 0;
	} 
    
    if (!vm.count("service")) {
		cout << "Missing argument: service" << endl << desc << endl;
		return 3;
	}

	if (vm.count("warn"))
		printInfo.warn = true;
	
	printInfo.service = vm["service"].as<wstring>();

	if (vm.count("debug"))
		debug = TRUE;
	
	return -1;
}

int printOutput(const printInfoStruct& printInfo) 
{
	if (debug)
		wcout << L"Constructing output string" << endl;

	wstring perf;
	state state = OK;

	if (!printInfo.ServiceState) {
		wcout << L"SERVICE CRITICAL NOT_FOUND | service=" << printInfo.ServiceState << ";!4;!4;1;7" << endl;
		return 3;
	}

	if (printInfo.ServiceState != 0x04) 
		printInfo.warn ? state = WARNING : state = CRITICAL;

	switch (state) {
	case OK:
		wcout << L"SERVICE OK RUNNING | service=4;!4;!4;1;7" << endl;
		break;
	case WARNING:
		wcout << L"SERVICE WARNING NOT_RUNNING | service=" << printInfo.ServiceState << ";!4;!4;1;7" << endl;
		break;
	case CRITICAL:
		wcout << L"SERVICE CRITICAL NOT_RUNNING | service=" << printInfo.ServiceState << ";!4;!4;1;7" << endl;
		break;
	}
    
	return state;
}

int ServiceStatus(const printInfoStruct& printInfo) 
{
	if (debug)
		wcout << L"Opening SC Manager" << endl;

	SC_HANDLE service_api = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if (service_api == NULL)
		goto die;

	LPBYTE lpServices = NULL;
	DWORD cbBufSize = 0;
	DWORD pcbBytesNeeded = NULL, ServicesReturned = NULL, ResumeHandle = NULL;

	if (debug)
		wcout << L"Creating service info structure" << endl;

	if (!EnumServicesStatusEx(service_api, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
		lpServices, cbBufSize, &pcbBytesNeeded, &ServicesReturned, &ResumeHandle, NULL)
		&& GetLastError() != ERROR_MORE_DATA) 
		goto die;

	lpServices = reinterpret_cast<LPBYTE>(new BYTE[pcbBytesNeeded]);
	cbBufSize = pcbBytesNeeded;

	if (!EnumServicesStatusEx(service_api, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
		lpServices, cbBufSize, &pcbBytesNeeded, &ServicesReturned, &ResumeHandle, NULL))
		goto die;

	LPENUM_SERVICE_STATUS_PROCESS pInfo = (LPENUM_SERVICE_STATUS_PROCESS)lpServices;
    
	if (debug)
		wcout << L"Traversing services" << endl;

	for (DWORD i = 0; i < ServicesReturned; i++) {
		if (debug)
			wcout << L"Comparing " << pInfo[i].lpServiceName << L" to " << printInfo.service << endl;

		if (!wcscmp(printInfo.service.c_str(), pInfo[i].lpServiceName)) {
			if (debug)
				wcout << L"Service " << pInfo[i].lpServiceName << L" = " << printInfo.service << ". Returning" << endl;

			int state = pInfo[i].ServiceStatusProcess.dwCurrentState;
			delete lpServices;
			return state;
		}
	}
	delete[] reinterpret_cast<LPBYTE>(lpServices);
	return 0;

die:
	die();
	if (lpServices)
		delete[] reinterpret_cast<LPBYTE>(lpServices);
	return -1;
}
