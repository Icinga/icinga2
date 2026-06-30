// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#define WIN32_LEAN_AND_MEAN

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <vector>
#include <map>
#include <windows.h>
#include <pdh.h>
#include <shlwapi.h>
#include <iostream>
#include <pdhmsg.h>
#include <winsock2.h>
#include <iphlpapi.h>

#define VERSION 1.2

namespace po = boost::program_options;

struct nInterface
{
	std::wstring name;
	LONG BytesInSec, BytesOutSec;
	nInterface(std::wstring p)
		: name(p)
	{ }
};

struct printInfoStruct
{
	threshold warn;
	threshold crit;
};

static bool l_Debug;
static bool l_NoISATAP;

static int parseArguments(int ac, WCHAR **av, po::variables_map& vm, printInfoStruct& printInfo)
{
	WCHAR namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	WCHAR *progName = PathFindFileName(namePath);

	po::options_description desc("Options");

	desc.add_options()
		("help,h", "print usage and exit")
		("version,V", "print version and exit")
		("debug,d", "Verbose/Debug output")
		("noisatap,n", "Don't show ISATAP interfaces in output")
		("warning,w", po::wvalue<std::wstring>(), "warning value")
		("critical,c", po::wvalue<std::wstring>(), "critical value")
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
			L"%s is a simple program to check a machines network performance.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tNETWORK WARNING 1131B/s | network=1131B;1000;7000;0\n\n"
			L"\"NETWORK\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"1131B/s\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. Performance data will only be displayed when\n"
			L"you set at least one threshold\n\n"
			L"This program will also print out additional performance data interface\n"
			L"by interface\n\n"
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
			L"All of these options work with the critical threshold \"-c\" too."
			, progName);
		std::cout << '\n';
		return 0;
	}

	if (vm.count("version"))
		std::cout << "Version: " << VERSION << '\n';

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
	l_NoISATAP = vm.count("noisatap") > 0;

	return -1;
}

static int printOutput(printInfoStruct& printInfo, const std::vector<nInterface>& vInterfaces, const std::map<std::wstring, std::wstring>& mapNames)
{
	if (l_Debug)
		std::wcout << L"Constructing output string" << '\n';

	long tIn = 0, tOut = 0;
	std::wstringstream tss;
	state state = OK;

	std::map<std::wstring, std::wstring>::const_iterator mapIt;
	std::wstring wsFriendlyName;

	for (std::vector<nInterface>::const_iterator it = vInterfaces.begin(); it != vInterfaces.end(); ++it) {
		tIn += it->BytesInSec;
		tOut += it->BytesOutSec;
		if (l_Debug)
			std::wcout << "Getting friendly name of " << it->name << '\n';
		mapIt = mapNames.find(it->name);
		if (mapIt != mapNames.end()) {
			if (l_Debug)
				std::wcout << "\tIs " << mapIt->second << '\n';
			wsFriendlyName = mapIt->second;
		} else {
			if (l_Debug)
				std::wcout << "\tNo friendly name found, using adapter name\n";
			wsFriendlyName = it->name;
		}
		if (wsFriendlyName.find(L"isatap") != std::wstring::npos && l_NoISATAP) {
			if (l_Debug)
				std::wcout << "\tSkipping isatap interface " << wsFriendlyName << "\n";
			continue;
		} else {
			boost::algorithm::replace_all(wsFriendlyName, "'", "''");
			tss << L"'" << wsFriendlyName << L"_in'=" << it->BytesInSec << L"B '" << wsFriendlyName << L"_out'=" << it->BytesOutSec << L"B ";
		}
	}

	if (printInfo.warn.rend(tIn + tOut))
		state = WARNING;
	if (printInfo.crit.rend(tIn + tOut))
		state = CRITICAL;

	std::wcout << "NETWORK ";

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

	std::wcout << " " << tIn + tOut << L"B/s | "
		<< L"'network'=" << tIn + tOut << L"B;" << printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";" << L"0; "
		<< L"'network_in'=" << tIn << L"B 'network_out'=" << tOut << L"B "
		<< tss.str() << '\n';

	return state;
}

static int check_network(std::vector<nInterface>& vInterfaces)
{

	if (l_Debug)
		std::wcout << L"Creating Query and adding counters" << '\n';

	PDH_FMT_COUNTERVALUE_ITEM *pDisplayValuesIn = NULL, *pDisplayValuesOut = NULL;

	PDH_HQUERY phQuery;
	PDH_STATUS err = PdhOpenQuery(NULL, NULL, &phQuery);
	if (!SUCCEEDED(err))
		goto die;

	const WCHAR *perfIn = L"\\Network Interface(*)\\Bytes Received/sec";
	PDH_HCOUNTER phCounterIn;
	err = PdhAddEnglishCounter(phQuery, perfIn, NULL, &phCounterIn);
	if (!SUCCEEDED(err))
		goto die;

	const WCHAR *perfOut = L"\\Network Interface(*)\\Bytes Sent/sec";
	PDH_HCOUNTER phCounterOut;
	err = PdhAddEnglishCounter(phQuery, perfOut, NULL, &phCounterOut);
	if (!SUCCEEDED(err))
		goto die;

	if (l_Debug)
		std::wcout << L"Collecting first batch of query data" << '\n';

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (l_Debug)
		std::wcout << L"Sleep for one second" << '\n';

	Sleep(1000);

	if (l_Debug)
		std::wcout << L"Collecting second batch of query data" << '\n';

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (l_Debug)
		std::wcout << L"Creating formatted counter arrays" << '\n';

	DWORD dwItemCount;
	DWORD dwBufferSizeIn = 0;
	err = PdhGetFormattedCounterArray(phCounterIn, PDH_FMT_LONG, &dwBufferSizeIn, &dwItemCount, pDisplayValuesIn);
	if (err == PDH_MORE_DATA || SUCCEEDED(err))
		pDisplayValuesIn = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(new BYTE[dwItemCount*dwBufferSizeIn]);
	else
		goto die;

	DWORD dwBufferSizeOut = 0;
	err = PdhGetFormattedCounterArray(phCounterOut, PDH_FMT_LONG, &dwBufferSizeOut, &dwItemCount, pDisplayValuesOut);
	if (err == PDH_MORE_DATA || SUCCEEDED(err))
		pDisplayValuesOut = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(new BYTE[dwItemCount*dwBufferSizeIn]);
	else
		goto die;

	err = PdhGetFormattedCounterArray(phCounterIn, PDH_FMT_LONG, &dwBufferSizeIn, &dwItemCount, pDisplayValuesIn);
	if (!SUCCEEDED(err))
		goto die;

	err = PdhGetFormattedCounterArray(phCounterOut, PDH_FMT_LONG, &dwBufferSizeOut, &dwItemCount, pDisplayValuesOut);
	if (!SUCCEEDED(err))
		goto die;

	if (l_Debug)
		std::wcout << L"Going over counter array" << '\n';

	for (DWORD i = 0; i < dwItemCount; i++) {
		nInterface iface{pDisplayValuesIn[i].szName};
		iface.BytesInSec = pDisplayValuesIn[i].FmtValue.longValue;
		iface.BytesOutSec = pDisplayValuesOut[i].FmtValue.longValue;
		vInterfaces.push_back(iface);

		if (l_Debug)
			std::wcout << L"Collected interface " << pDisplayValuesIn[i].szName << '\n';
	}

	if (l_Debug)
		std::wcout << L"Finished collection. Cleaning up and returning" << '\n';

	if (phQuery)
		PdhCloseQuery(phQuery);

	delete reinterpret_cast<BYTE *>(pDisplayValuesIn);
	delete reinterpret_cast<BYTE *>(pDisplayValuesOut);

	return -1;
die:
	printErrorInfo(err);
	if (phQuery)
		PdhCloseQuery(phQuery);

	delete reinterpret_cast<BYTE *>(pDisplayValuesIn);
	delete reinterpret_cast<BYTE *>(pDisplayValuesOut);

	return 3;
}

static bool mapSystemNamesToFamiliarNames(std::map<std::wstring, std::wstring>& mapNames)
{
	/*
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
	PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
	PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
	PIP_ADAPTER_DNS_SERVER_ADDRESS pDnsServer = NULL;
	PIP_ADAPTER_PREFIX pPrefix = NULL;
	*/
	ULONG outBufLen = 15000; //15KB as suggestet by msdn of GetAdaptersAddresses

	if (l_Debug)
		std::wcout << "Mapping adapter system names to friendly names\n";

	PIP_ADAPTER_ADDRESSES pAddresses;

	unsigned int Iterations = 0;
	DWORD dwRetVal = 0;

	do {
		pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(new BYTE[outBufLen]);

		dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen);

		if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
			delete[]pAddresses;
			pAddresses = NULL;
		} else
			break;
	} while (++Iterations < 3);

	if (dwRetVal != NO_ERROR) {
		std::wcout << "Failed to collect friendly adapter names\n";
		delete[]pAddresses;
		return false;
	}

	for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses; pCurrAddresses = pCurrAddresses->Next) {
		if (l_Debug)
			std::wcout << "Got: " << pCurrAddresses->Description << " -- " << pCurrAddresses->FriendlyName << '\n';

		mapNames[pCurrAddresses->Description] = pCurrAddresses->FriendlyName;
	}

	delete[]pAddresses;
	return true;
}

int wmain(int argc, WCHAR **argv)
{
	std::vector<nInterface> vInterfaces;
	std::map<std::wstring, std::wstring> mapNames;
	printInfoStruct printInfo;
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);

	if (ret != -1)
		return ret;

	if (!mapSystemNamesToFamiliarNames(mapNames))
		return 3;

	ret = check_network(vInterfaces);
	if (ret != -1)
		return ret;

	return printOutput(printInfo, vInterfaces, mapNames);
}
