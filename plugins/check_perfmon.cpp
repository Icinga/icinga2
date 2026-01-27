// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <vector>
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <shlwapi.h>

#define VERSION 1.0

namespace po = boost::program_options;

struct printInfoStruct
{
	threshold tWarn;
	threshold tCrit;
	std::wstring wsFullPath;
	double dValue;
	DWORD dwPerformanceWait = 1000;
	DWORD dwRequestedType = PDH_FMT_DOUBLE;
};

static bool parseArguments(const int ac, WCHAR **av, po::variables_map& vm, printInfoStruct& printInfo)
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
		("perf-syntax", po::wvalue<std::wstring>(), "Use this string as name for the performance counter (graphite compatibility)")
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
		return false;
	}

	if (vm.count("version")) {
		std::wcout << "Version: " << VERSION << '\n';
		return false;
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
		return false;
	}

	if (vm.count("warning")) {
		try {
			printInfo.tWarn = threshold(vm["warning"].as<std::wstring>());
		} catch (const std::invalid_argument& e) {
			std::wcout << e.what() << '\n';
			return false;
		}
	}

	if (vm.count("critical")) {
		try {
			printInfo.tCrit = threshold(vm["critical"].as<std::wstring>());
		} catch (const std::invalid_argument& e) {
			std::wcout << e.what() << '\n';
			return false;
		}
	}

	if (vm.count("fmt-countertype")) {
		if (!vm["fmt-countertype"].as<std::wstring>().compare(L"int64"))
			printInfo.dwRequestedType = PDH_FMT_LARGE;
		else if (!vm["fmt-countertype"].as<std::wstring>().compare(L"long"))
			printInfo.dwRequestedType = PDH_FMT_LONG;
		else if (vm["fmt-countertype"].as<std::wstring>().compare(L"double")) {
			std::wcout << "Unknown value type " << vm["fmt-countertype"].as<std::wstring>() << '\n';
			return false;
		}
	}

	if (vm.count("performance-counter"))
		printInfo.wsFullPath = vm["performance-counter"].as<std::wstring>();

	if (vm.count("performance-wait"))
		printInfo.dwPerformanceWait = vm["performance-wait"].as<DWORD>();

	return true;
}

static bool getInstancesAndCountersOfObject(const std::wstring& wsObject,
	std::vector<std::wstring>& vecInstances, std::vector<std::wstring>& vecCounters)
{
	DWORD dwCounterListLength = 0, dwInstanceListLength = 0;

	if (PdhEnumObjectItems(NULL, NULL, wsObject.c_str(),
		NULL, &dwCounterListLength, NULL,
		&dwInstanceListLength, PERF_DETAIL_WIZARD, 0) != PDH_MORE_DATA)
		return false;

	std::vector<WCHAR> mszCounterList(dwCounterListLength + 1);
	std::vector<WCHAR> mszInstanceList(dwInstanceListLength + 1);

	if (FAILED(PdhEnumObjectItems(NULL, NULL, wsObject.c_str(),
		mszCounterList.data(), &dwCounterListLength, mszInstanceList.data(),
		&dwInstanceListLength, PERF_DETAIL_WIZARD, 0))) {
		return false;
	}

	if (dwInstanceListLength) {
		std::wstringstream wssInstanceName;

		// XXX: is the "- 1" correct?
		for (DWORD c = 0; c < dwInstanceListLength - 1; ++c) {
			if (mszInstanceList[c])
				wssInstanceName << mszInstanceList[c];
			else {
				vecInstances.push_back(wssInstanceName.str());
				wssInstanceName.str(L"");
			}
		}
	}

	if (dwCounterListLength) {
		std::wstringstream wssCounterName;

		// XXX: is the "- 1" correct?
		for (DWORD c = 0; c < dwCounterListLength - 1; ++c) {
			if (mszCounterList[c])
				wssCounterName << mszCounterList[c];
			else {
				vecCounters.push_back(wssCounterName.str());
				wssCounterName.str(L"");
			}
		}
	}

	return true;
}

static void printPDHError(PDH_STATUS status)
{
	HMODULE hPdhLibrary = NULL;
	LPWSTR pMessage = NULL;

	hPdhLibrary = LoadLibrary(L"pdh.dll");
	if (!hPdhLibrary) {
		std::wcout << "LoadLibrary failed with " << GetLastError() << '\n';
		return;
	}

	if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
		hPdhLibrary, status, 0, (LPWSTR)&pMessage, 0, NULL)) {
		FreeLibrary(hPdhLibrary);
		std::wcout << "Format message failed with " << std::hex << GetLastError() << '\n';
		return;
	}

	FreeLibrary(hPdhLibrary);

	std::wcout << pMessage << '\n';
	LocalFree(pMessage);
}

static void printObjects()
{
	DWORD dwBufferLength = 0;
	PDH_STATUS status =	PdhEnumObjects(NULL, NULL, NULL,
		&dwBufferLength, PERF_DETAIL_WIZARD, FALSE);
	//HEX HEX! Only a Magicians gets all the info he wants, and only Microsoft knows what that means

	if (status != PDH_MORE_DATA) {
		printPDHError(status);
		return;
	}

	std::vector<WCHAR> mszObjectList(dwBufferLength + 2);
	status = PdhEnumObjects(NULL, NULL, mszObjectList.data(),
		&dwBufferLength, PERF_DETAIL_WIZARD, FALSE);

	if (FAILED(status)) {
		printPDHError(status);
		return;
	}

	DWORD c = 0;

	while (++c < dwBufferLength) {
		if (mszObjectList[c] == '\0')
			std::wcout << '\n';
		else
			std::wcout << mszObjectList[c];
	}
}

static void printObjectInfo(const printInfoStruct& pI)
{
	if (pI.wsFullPath.empty()) {
		std::wcout << "No object given!\n";
		return;
	}

	std::vector<std::wstring> vecInstances, vecCounters;

	if (!getInstancesAndCountersOfObject(pI.wsFullPath, vecInstances, vecCounters)) {
		std::wcout << "Could not enumerate instances and counters of " << pI.wsFullPath << '\n'
			<< "Make sure it exists!\n";
		return;
	}

	std::wcout << "Instances of " << pI.wsFullPath << ":\n";
	if (vecInstances.empty())
		std::wcout << "> Has no instances\n";
	else {
		for (const auto& instance : vecInstances)
			std::wcout << "> " << instance << '\n';
	}
	std::wcout << std::endl;

	std::wcout << "Performance Counters of " << pI.wsFullPath << ":\n";
	if (vecCounters.empty())
		std::wcout << "> Has no counters\n";
	else {
		for (const auto& counter : vecCounters)
			std::wcout << "> " << counter << "\n";
	}
	std::wcout << std::endl;
}

bool QueryPerfData(printInfoStruct& pI)
{
	PDH_HQUERY hQuery = NULL;
	PDH_HCOUNTER hCounter = NULL;
	DWORD dwBufferSize = 0, dwItemCount = 0;

	if (pI.wsFullPath.empty()) {
		std::wcout << "No performance counter path given!\n";
		return false;
	}

	PDH_FMT_COUNTERVALUE_ITEM *pDisplayValues = NULL;

	PDH_STATUS status = PdhOpenQuery(NULL, NULL, &hQuery);
	if (FAILED(status))
		goto die;

	status = PdhAddEnglishCounter(hQuery, pI.wsFullPath.c_str(), NULL, &hCounter);

	if (FAILED(status))
		status = PdhAddCounter(hQuery, pI.wsFullPath.c_str(), NULL, &hCounter);

	if (FAILED(status))
		goto die;

	status = PdhCollectQueryData(hQuery);
	if (FAILED(status))
		goto die;

	/*
	* Most counters need two queries to provide a value.
	* Those which need only one will return the second.
	*/
	Sleep(pI.dwPerformanceWait);

	status = PdhCollectQueryData(hQuery);
	if (FAILED(status))
		goto die;

	status = PdhGetFormattedCounterArray(hCounter, pI.dwRequestedType, &dwBufferSize, &dwItemCount, NULL);
	if (status != PDH_MORE_DATA)
		goto die;

	pDisplayValues = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(new BYTE[dwBufferSize]);
	status = PdhGetFormattedCounterArray(hCounter, pI.dwRequestedType, &dwBufferSize, &dwItemCount, pDisplayValues);

	if (FAILED(status))
		goto die;

	switch (pI.dwRequestedType) {
	case (PDH_FMT_LONG):
		pI.dValue = pDisplayValues[0].FmtValue.longValue;
		break;
	case (PDH_FMT_LARGE):
		pI.dValue = (double) pDisplayValues[0].FmtValue.largeValue;
		break;
	default:
		pI.dValue = pDisplayValues[0].FmtValue.doubleValue;
		break;
	}

	delete[]pDisplayValues;

	return true;

die:
	printPDHError(status);
	delete[]pDisplayValues;
	return false;
}

static int printOutput(const po::variables_map& vm, printInfoStruct& pi)
{
	std::wstringstream wssPerfData;

	if (vm.count("perf-syntax"))
		wssPerfData << "'" << vm["perf-syntax"].as<std::wstring>() << "'=";
	else
		wssPerfData << "'" << pi.wsFullPath << "'=";

	wssPerfData << pi.dValue << ';' << pi.tWarn.pString() << ';' << pi.tCrit.pString() << ";;";

	if (pi.tCrit.rend(pi.dValue)) {
		std::wcout << "PERFMON CRITICAL for '" << (vm.count("perf-syntax") ? vm["perf-syntax"].as<std::wstring>() : pi.wsFullPath)
			<< "' = " << pi.dValue << " | " << wssPerfData.str() << "\n";
		return 2;
	}

	if (pi.tWarn.rend(pi.dValue)) {
		std::wcout << "PERFMON WARNING for '" << (vm.count("perf-syntax") ? vm["perf-syntax"].as<std::wstring>() : pi.wsFullPath)
			<< "' = " << pi.dValue << " | " << wssPerfData.str() << "\n";
		return 1;
	}

	std::wcout << "PERFMON OK for '" << (vm.count("perf-syntax") ? vm["perf-syntax"].as<std::wstring>() : pi.wsFullPath)
		<< "' = " << pi.dValue << " | " << wssPerfData.str() << "\n";

	return 0;
}

int wmain(int argc, WCHAR **argv)
{
	po::variables_map variables_map;
	printInfoStruct stPrintInfo;
	if (!parseArguments(argc, argv, variables_map, stPrintInfo))
		return 3;

	if (variables_map.count("print-objects")) {
		printObjects();
		return 0;
	}

	if (variables_map.count("print-object-info")) {
		printObjectInfo(stPrintInfo);
		return 0;
	}

	if (QueryPerfData(stPrintInfo))
		return printOutput(variables_map, stPrintInfo);
	else
		return 3;
}
