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

#define VERSION 1.1

namespace po = boost::program_options;

using std::wcout; using std::endl;
using std::cout; using std::wstring;

static BOOL debug;

struct printInfoStruct 
{
	bool warn;
	DWORD ServiceState;
	wstring service;
};

static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(const printInfoStruct&);
static DWORD ServiceStatus(const printInfoStruct&);

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
		wcout << L"SERVICE CRITICAL NOTFOUND | service=" << printInfo.ServiceState << ";!4;!4;1;7" << endl;
		return 3;
	}

	if (printInfo.ServiceState != 0x04) 
		printInfo.warn ? state = WARNING : state = CRITICAL;

	switch (state) {
	case OK:
		wcout << L"SERVICE OK RUNNING | service=4;!4;!4;1;7" << endl;
		break;
	case WARNING:
		wcout << L"SERVICE WARNING NOT RUNNING | service=" << printInfo.ServiceState << ";!4;!4;1;7" << endl;
		break;
	case CRITICAL:
		wcout << L"SERVICE CRITICAL NOT RUNNING | service=" << printInfo.ServiceState << ";!4;!4;1;7" << endl;
		break;
	}

	return state;
}

DWORD ServiceStatus(const printInfoStruct& printInfo) 
{
	SC_HANDLE hSCM;
	SC_HANDLE hService;
	DWORD cbBufSize;
	LPBYTE lpBuf = NULL;

	if (debug)
		wcout << L"Opening SC Manager" << endl;

	hSCM = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (hSCM == NULL)
		goto die;

	if (debug)
		wcout << L"Getting Service Information" << endl;

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
	die();
	if (hSCM)
		CloseServiceHandle(hSCM);
	if (hService)
		CloseServiceHandle(hService);
	delete [] lpBuf;

	return -1;
}
