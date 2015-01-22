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
#include <tlhelp32.h>
#include <iostream>

#include "thresholds.h"

#include "boost/program_options.hpp"

#define VERSION 1.0

namespace po = boost::program_options;

using std::endl; using std::wstring; using std::wcout;
using std::cout;

struct printInfoStruct 
{
	threshold warn, crit;
	wstring user;
};

static int countProcs();
static int countProcs(const wstring);
static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(const int, printInfoStruct&);

int wmain(int argc, wchar_t **argv) 
{
	po::variables_map vm;
	printInfoStruct printInfo = { };

	int r = parseArguments(argc, argv, vm, printInfo);
    
	if (r != -1)
		return r;

	if(!printInfo.user.empty())
		return printOutput(countProcs(printInfo.user), printInfo);

	return printOutput(countProcs(), printInfo);
}

int printOutput(const int numProcs, printInfoStruct& printInfo) 
{
	state state = OK;

	if (printInfo.warn.rend(numProcs))
		state = WARNING;

	if (printInfo.crit.rend(numProcs))
		state = CRITICAL;
	
	switch (state) {
	case OK:
		wcout << L"PROCS OK " << numProcs << L"|procs=" << numProcs << L";" 
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0" << endl;
		break;
	case WARNING:
		wcout << L"PROCS WARNING " << numProcs << L"|procs=" << numProcs << L";"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0" << endl;
		break;
	case CRITICAL:
		wcout << L"PROCS CRITICAL " << numProcs << L"|procs=" << numProcs << L";"
			<< printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";0" << endl;
		break;
	}

	return state;
}

int parseArguments(int ac, wchar_t **av, po::variables_map& vm, printInfoStruct& printInfo) 
{
	wchar_t namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	wchar_t *progName = PathFindFileName(namePath);

	po::options_description desc;

	desc.add_options()
		("h", "print help message and exit")
		("help", "print verbose help and exit")
		("version,v", "print version and exit")
		("warning,w", po::wvalue<wstring>(), "warning threshold")
		("critical,c", po::wvalue<wstring>(), "critical threshold")
		("user,u", po::wvalue<wstring>(), "count only processes by user [arg]")
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
		std::cout << e.what() << endl << desc << endl;
		return 3;
	}

	if (vm.count("h")) {
		std::cout << desc << endl;
		return 0;
	}
    
	if (vm.count("help")) {
		wcout << progName << " Help\n\tVersion: " << VERSION << endl;
		wprintf(
			L"%s is a simple program to check a machines processes.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tPROCS WARNING 67|load=67;50;90;0\n\n"
			L"\"PROCS\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"67\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. Performance data will only be displayed when\n"
			L"you set at least one threshold\n\n"
			L"For \"-user\" option keep in mind you need root to see other users processes\n\n"
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
    
	if (vm.count("version")) {
		std::cout << "Version: " << VERSION << endl;
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

	if (vm.count("user")) 
		printInfo.user = vm["user"].as<wstring>();

	return -1;
}

int countProcs() 
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return -1;

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32)) {
		CloseHandle(hProcessSnap);
		return -1;
	}

	int numProcs = 0;

	do {
		++numProcs;
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return numProcs;
}

int countProcs(const wstring user) 
{
	const wchar_t *wuser = user.c_str();
	int numProcs = 0;

	HANDLE hProcessSnap, hProcess = NULL, hToken = NULL;
	PROCESSENTRY32 pe32;
	DWORD dwReturnLength, dwAcctName, dwDomainName;
	PTOKEN_USER pSIDTokenUser = NULL;
	SID_NAME_USE sidNameUse;
	LPWSTR AcctName, DomainName;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		goto die;

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32))
		goto die;

	do {
		//get ProcessToken
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
		if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) 
			//Won't count pid 0 (system idle) and 4/8 (Sytem)
			continue;

		//Get dwReturnLength in first call
		dwReturnLength = 1;
		if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwReturnLength)
			&& GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
			continue;

		pSIDTokenUser = reinterpret_cast<PTOKEN_USER>(new BYTE[dwReturnLength]);
		memset(pSIDTokenUser, 0, dwReturnLength);

		if (!pSIDTokenUser)
			continue;

		//write Info in pSIDTokenUser
		if (!GetTokenInformation(hToken, TokenUser, pSIDTokenUser, dwReturnLength, NULL))
			continue;

		AcctName = NULL;
		DomainName = NULL;
		dwAcctName = 1;
		dwDomainName = 1;
		//get dwAcctName and dwDomainName size
		if (!LookupAccountSid(NULL, pSIDTokenUser->User.Sid, AcctName,
			(LPDWORD)&dwAcctName, DomainName, (LPDWORD)&dwDomainName, &sidNameUse)
			&& GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			continue;
		
		AcctName = reinterpret_cast<LPWSTR>(new WCHAR[dwAcctName]);
		DomainName = reinterpret_cast<LPWSTR>(new WCHAR[dwDomainName]);

		if (!AcctName || !DomainName)
			continue;
		
		if (!LookupAccountSid(NULL, pSIDTokenUser->User.Sid, AcctName,
			(LPDWORD)&dwAcctName, DomainName, (LPDWORD)&dwDomainName, &sidNameUse))
			continue;

		if (!wcscmp(AcctName, wuser)) 
			++numProcs;
		
		delete[] reinterpret_cast<LPWSTR>(AcctName);
		delete[] reinterpret_cast<LPWSTR>(DomainName);

	} while (Process32Next(hProcessSnap, &pe32));
	

die:
	if (hProcessSnap)
		CloseHandle(hProcessSnap);
	if (hProcess)
		CloseHandle(hProcess);
	if (hToken)
		CloseHandle(hToken);
	if (pSIDTokenUser)
		delete[] reinterpret_cast<PTOKEN_USER>(pSIDTokenUser);
	return numProcs;
}