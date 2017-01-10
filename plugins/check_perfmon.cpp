/******************************************************************************
* Icinga 2                                                                   *
* Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include <iostream>
#include <vector>

#include "check_perfmon.h"

#define VERSION 1.0

namespace po = boost::program_options;

INT wmain(INT argc, WCHAR **argv)
{
	po::variables_map variables_map;
	printInfoStruct stPrintInfo;
	if (!ParseArguments(argc, argv, variables_map, stPrintInfo))
		return 3;

	if (variables_map.count("print-objects")) {
		PrintObjects();
		return 0;
	}

	if (variables_map.count("print-object-info")) {
		PrintObjectInfo(stPrintInfo);
		return 0;
	}

	if (QueryPerfData(stPrintInfo))
		return PrintOutput(variables_map, stPrintInfo);
	else
		return 3;
}

BOOL ParseArguments(CONST INT ac, WCHAR **av, po::variables_map& vm, printInfoStruct& printInfo)
{
	WCHAR szNamePath[MAX_PATH + 1];
	GetModuleFileName(NULL, szNamePath, MAX_PATH);
	WCHAR *szProgName = PathFindFileName(szNamePath);

	po::options_description desc("Options");
	desc.add_options()
		("help,h", "Print help page and exit")
		("version,V", "Print version and exit")
		("warning,w", po::wvalue<std::wstring>(), "Warning thershold")
		("critical,c", po::wvalue<std::wstring>(), "Critical threshold")
		("performance-counter,P", po::wvalue<std::wstring>(), "The performance counter string to use")
		("performance-wait", po::value<DWORD>(), "Sleep in milliseconds between the two perfomance querries (Default: 1000ms)")
		("fmt-countertype", po::wvalue<std::wstring>(), "Value type of counter: 'double'(default), 'long', 'int64'")
		("print-objects", "Prints all available objects to console")
		("print-object-info", "Prints all available instances and counters of --performance-counter, do not use a full perfomance counter string here")
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
		std::cout << e.what() << '\n' << desc << '\n';
		return FALSE;
	}

	if (vm.count("version")) {
		std::wcout << "Version: " << VERSION << '\n';
		return FALSE;
	}

	if (vm.count("help")) {
		std::wcout << szProgName << " Help\n\tVersion: " << VERSION << '\n';
		wprintf(
			L"%s runs a check against a performance counter.\n"
			L"You can use the following options to define its behaviour:\n\n", szProgName);
		std::cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tPERFMON CRITICAL \"\\Processor(_Total)\\%% Idle Time\" = 40.34 | "
			L"perfmon=40.34;20;40;; \"\\Processor(_Total)\\%% Idle Time\"=40.34\n\n"
			L"\"tPERFMON\" being the type of the check, \"CRITICAL\" the returned status\n"
			L"and \"40.34\" is the performance counters value.\n"
			L"%s' exit codes denote the following:\n"
			L" 0\tOK,\n\tNo Thresholds were exceeded\n"
			L" 1\tWARNING,\n\tThe warning was broken, but not the critical threshold\n"
			L" 2\tCRITICAL,\n\tThe critical threshold was broken\n"
			L" 3\tUNKNOWN, \n\tNo check could be performed\n\n"
			, szProgName);
		return 0;
	}

	if (vm.count("warning")) {
		try {
			printInfo.tWarn = threshold(vm["warning"].as<std::wstring>());
		} catch (std::invalid_argument& e) {
			std::wcout << e.what() << '\n';
			return FALSE;
		}
	}

	if (vm.count("critical")) {
		try {
			printInfo.tCrit = threshold(vm["critical"].as<std::wstring>());
		} catch (std::invalid_argument& e) {
			std::wcout << e.what() << '\n';
			return FALSE;
		}
	}

	if (vm.count("fmt-countertype")) {
		if (!vm["fmt-countertype"].as<std::wstring>().compare(L"int64"))
			printInfo.dwRequestedType = PDH_FMT_LARGE;
		else if (!vm["fmt-countertype"].as<std::wstring>().compare(L"long"))
			printInfo.dwRequestedType = PDH_FMT_LONG;
		else if (vm["fmt-countertype"].as<std::wstring>().compare(L"double")) {
			std::wcout << "Unknown value type " << vm["fmt-countertype"].as<std::wstring>() << '\n';
			return FALSE;
		}
	}

	if (vm.count("performance-counter"))
		printInfo.wsFullPath = vm["performance-counter"].as<std::wstring>();

	if (vm.count("performance-wait"))
		printInfo.dwPerformanceWait = vm["performance-wait"].as<DWORD>();

	return TRUE;
}

BOOL GetIntstancesAndCountersOfObject(CONST std::wstring wsObject, 
									 std::vector<std::wstring>& vecInstances, 
									 std::vector<std::wstring>& vecCounters)
{
	LPWSTR szDataSource = NULL, szMachineName = NULL,
		mszCounterList = NULL, mszInstanceList = NULL;
	DWORD dwCounterListLength = 0, dwInstanceListLength = 0;

	std::wstringstream wssInstanceName, wssCounterName;
	LPWSTR szObjectName = new WCHAR[wsObject.length() + 1];
	StrCpyW(szObjectName, wsObject.c_str());

	PDH_STATUS status =
		PdhEnumObjectItems(szDataSource, szMachineName, szObjectName,
		mszCounterList, &dwCounterListLength, mszInstanceList,
		&dwInstanceListLength, PERF_DETAIL_WIZARD, 0);

	if (status != PDH_MORE_DATA) {
		delete[]szObjectName;
		return FALSE;
	}

	mszCounterList = new WCHAR[dwCounterListLength + 1];
	mszInstanceList = new WCHAR[dwInstanceListLength + 1];

	status = PdhEnumObjectItems(szDataSource, szMachineName, szObjectName,
								mszCounterList, &dwCounterListLength, mszInstanceList,
								&dwInstanceListLength, PERF_DETAIL_WIZARD, 0);

	if (FAILED(status)) {
		delete[]mszCounterList;
		delete[]mszInstanceList;
		delete[]szObjectName;
		return FALSE;
	}

	if (dwInstanceListLength) {
		for (DWORD c = 0; c < dwInstanceListLength-1; ++c) {
			if (mszInstanceList[c])
				wssInstanceName << mszInstanceList[c];
			else {
				vecInstances.push_back(wssInstanceName.str());
				wssInstanceName.str(L"");
			}
		}
	}

	if (dwCounterListLength) {
		for (DWORD c = 0; c < dwCounterListLength-1; ++c) {
			if (mszCounterList[c]) {
				wssCounterName << mszCounterList[c];
			} else {
				vecCounters.push_back(wssCounterName.str());
				wssCounterName.str(L"");
			}
		}
	}

	delete[]mszCounterList;
	delete[]mszInstanceList;
	delete[]szObjectName;

	return TRUE;
}

VOID PrintObjects()
{
	LPWSTR szDataSource = NULL, szMachineName = NULL, mszObjectList = NULL;
	DWORD dwBufferLength = 0;
	PDH_STATUS status =
		PdhEnumObjects(szDataSource, szMachineName, mszObjectList,
		&dwBufferLength, PERF_DETAIL_WIZARD, FALSE);
	//HEX HEX! Only a Magicians gets all the info he wants, and only Microsoft knows what that means

	if (status != PDH_MORE_DATA)
		goto die;

	mszObjectList = new WCHAR[dwBufferLength + 2];
	status = PdhEnumObjects(szDataSource, szMachineName, mszObjectList,
							&dwBufferLength, PERF_DETAIL_WIZARD, FALSE);

	if (FAILED(status))
		goto die;

	DWORD c = 0;

	while (++c < dwBufferLength) {
		if (mszObjectList[c] == '\0')
			std::wcout << '\n';
		else
			std::wcout << mszObjectList[c];
	}

	delete[]mszObjectList;
	return;

die:
	FormatPDHError(status);
	delete[]mszObjectList;
}

VOID PrintObjectInfo(CONST printInfoStruct& pI)
{
	if (pI.wsFullPath.empty()) {
		std::wcout << "No object given!\n";
		return;
	}

	std::vector<std::wstring> vecInstances, vecCounters;

	if (!GetIntstancesAndCountersOfObject(pI.wsFullPath, vecInstances, vecCounters)) {
		std::wcout << "Could not enumerate instances and counters of " << pI.wsFullPath << '\n'
		    << "Make sure it exists!\n";
		return;
	}

	std::wcout << "Instances of " << pI.wsFullPath << ":\n";
	if (vecInstances.empty())
		std::wcout << "> Has no instances\n";
	else {
		for (std::vector<std::wstring>::iterator it = vecInstances.begin();
			 it != vecInstances.end(); ++it) {
			std::wcout << "> " << *it << '\n';
		}
	}
	std::wcout << std::endl;

	std::wcout << "Performance Counters of " << pI.wsFullPath << ":\n";
	if (vecCounters.empty())
		std::wcout << "> Has no counters\n";
	else {
		for (std::vector<std::wstring>::iterator it = vecCounters.begin();
			 it != vecCounters.end(); ++it) {
			std::wcout << "> " << *it << '\n';
		}
	}
	std::wcout << std::endl;
}

BOOL QueryPerfData(printInfoStruct& pI)
{
	PDH_HQUERY hQuery = NULL;
	PDH_HCOUNTER hCounter = NULL;
	PDH_FMT_COUNTERVALUE_ITEM *pDisplayValues = NULL;
	DWORD dwBufferSize = 0, dwItemCount = 0;

	if (pI.wsFullPath.empty()) {
		std::wcout << "No performance counter path given!\n";
		return FALSE;
	}

	PDH_STATUS status = PdhOpenQuery(NULL, NULL, &hQuery);
	if (FAILED(status))
		goto die;

	status = PdhAddCounter(hQuery, pI.wsFullPath.c_str(), NULL, &hCounter);
	if (FAILED(status))
		goto die;

	status = PdhCollectQueryData(hQuery);
	if (FAILED(status))
		goto die;

	/* 
	/* Most counters need two queries to provide a value.
	/* Those which need only one will return the second.
	 */
	Sleep(pI.dwPerformanceWait);

	status = PdhCollectQueryData(hQuery);
	if (FAILED(status))
		goto die;

	status = PdhGetFormattedCounterArray(hCounter, pI.dwRequestedType,
										 &dwBufferSize, &dwItemCount, pDisplayValues);
	if (status != PDH_MORE_DATA)
		goto die;

	pDisplayValues = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(new BYTE[dwBufferSize]);
	status = PdhGetFormattedCounterArray(hCounter, pI.dwRequestedType,
										 &dwBufferSize, &dwItemCount, pDisplayValues);

	if (FAILED(status))
		goto die;

	switch (pI.dwRequestedType)
	{
	case (PDH_FMT_LONG):
		pI.dValue = pDisplayValues[0].FmtValue.longValue;
		break;
	case (PDH_FMT_LARGE):
		pI.dValue = pDisplayValues[0].FmtValue.largeValue;
		break;
	default:
		pI.dValue = pDisplayValues[0].FmtValue.doubleValue;
		break;
	}

	delete[]pDisplayValues;

	return TRUE;

die:
	FormatPDHError(status);
	delete[]pDisplayValues;
	return FALSE;
}

INT PrintOutput(CONST po::variables_map& vm, printInfoStruct& pi)
{
	std::wstringstream wssPerfData;
	wssPerfData << "\"" << pi.wsFullPath << "\"=" << pi.dValue << ';'
		<< pi.tWarn.pString() << ';' << pi.tCrit.pString() << ";;";

	if (pi.tCrit.rend(pi.dValue)) {
		std::wcout << "PERFMON CRITICAL \"" << pi.wsFullPath << "\" = "
			<< pi.dValue << " | " << wssPerfData.str() << '\n';
		return 2;
	}

	if (pi.tWarn.rend(pi.dValue)) {
		std::wcout << "PERFMON WARNING \"" << pi.wsFullPath << "\" = "
			<< pi.dValue << " | " << wssPerfData.str() << '\n';
		return 1;
	}

	std::wcout << "PERFMON OK \"" << pi.wsFullPath << "\" = "
		<< pi.dValue << " | " << wssPerfData.str() << '\n';
	return 0;
}

VOID FormatPDHError(PDH_STATUS status)
{
	HANDLE hPdhLibrary = NULL;
	LPWSTR pMessage = NULL;

	hPdhLibrary = LoadLibrary(L"pdh.dll");
	if (NULL == hPdhLibrary) {
		std::wcout << "LoadLibrary failed with " << GetLastError() << '\n';
		return;
	}

	if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
		hPdhLibrary, status, 0, (LPWSTR)&pMessage, 0, NULL)) {
		std::wcout << "Format message failed with " << std::hex << GetLastError() << '\n';
		return;
	}

	std::wcout << pMessage << '\n';
	LocalFree(pMessage);
}
