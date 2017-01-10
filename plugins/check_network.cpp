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

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <Pdh.h>
#include <Shlwapi.h>
#include <iostream>
#include <pdhmsg.h>
#include <WinSock2.h>
#include <map>

#include <IPHlpApi.h>

#include "check_network.h"
#include "boost/algorithm/string/replace.hpp"

#define VERSION 1.2

namespace po = boost::program_options;

static BOOL debug = FALSE;
static BOOL noisatap = FALSE;

INT wmain(INT argc, WCHAR **argv) 
{
	std::vector<nInterface> vInterfaces;
	std::map<std::wstring, std::wstring> mapNames;
	printInfoStruct printInfo{};
	po::variables_map vm;

	INT ret = parseArguments(argc, argv, vm, printInfo);

	if (ret != -1)
		return ret;

	if (!mapSystemNamesToFamiliarNames(mapNames))
		return 3;

	ret = check_network(vInterfaces);
	if (ret != -1)
		return ret;
		
	return printOutput(printInfo, vInterfaces, mapNames);
}

INT parseArguments(INT ac, WCHAR **av, po::variables_map& vm, printInfoStruct& printInfo) 
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

	po::basic_command_line_parser<WCHAR> parser(ac, av);

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
		std::cout << "Version: " << VERSION << '\n';

	if (vm.count("warning")) {
		try {
			printInfo.warn = threshold(vm["warning"].as<std::wstring>());
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}
	if (vm.count("critical")) {
		try {
			printInfo.crit = threshold(vm["critical"].as<std::wstring>());
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}
	
	if (vm.count("debug"))
		debug = TRUE;

	if (vm.count("noisatap"))
		noisatap = TRUE;

	return -1;
}

INT printOutput(printInfoStruct& printInfo, CONST std::vector<nInterface>& vInterfaces, CONST std::map<std::wstring, std::wstring>& mapNames)
{
	if (debug)
		std::wcout << L"Constructing output string" << '\n';

	long tIn = 0, tOut = 0;
	std::wstringstream tss, perfDataFirst;
	state state = OK;

	std::map<std::wstring, std::wstring>::const_iterator mapIt;
	std::wstring wsFriendlyName;

	for (std::vector<nInterface>::const_iterator it = vInterfaces.begin(); it != vInterfaces.end(); ++it) {
		tIn += it->BytesInSec;
		tOut += it->BytesOutSec;
		if (debug)
			std::wcout << "Getting friendly name of " << it->name << '\n';
		mapIt = mapNames.find(it->name);
		if (mapIt != mapNames.end()) {
			if (debug)
				std::wcout << "\tIs " << mapIt->second << '\n';
			wsFriendlyName = mapIt->second;
		} else {
			if (debug)
				std::wcout << "\tNo friendly name found, using adapter name\n";
			wsFriendlyName = it->name;
		}
		if(wsFriendlyName.find(L"isatap") != std::wstring::npos && noisatap) {
			if (debug)
				std::wcout << "\tSkipping isatap interface " << wsFriendlyName << "\n";
			continue;
		}
		else
		{
			boost::algorithm::replace_all(wsFriendlyName, "'", "''");
			tss << L"\'" << wsFriendlyName << L"_in\'=" << it->BytesInSec << L"B \'" << wsFriendlyName << L"_out\'=" << it->BytesOutSec << L"B ";
		}
	}

	if (printInfo.warn.rend(tIn + tOut))
		state = WARNING;
	if (printInfo.crit.rend(tIn + tOut))
		state = CRITICAL;

	perfDataFirst << L"network=" << tIn + tOut << L"B;" << printInfo.warn.pString() << L";" << printInfo.crit.pString() << L";" << L"0; ";

	switch (state) {
	case OK:
		std::wcout << L"NETWORK OK " << tIn + tOut << L"B/s | " << perfDataFirst.str() << tss.str() << '\n';
		break;
	case WARNING:
		std::wcout << L"NETWORK WARNING " << tIn + tOut << L"B/s | " << perfDataFirst.str() << tss.str() << '\n';
		break;
	case CRITICAL:
		std::wcout << L"NETWORK CRITICAL " << tIn + tOut << L"B/s | " << perfDataFirst.str() << tss.str() << '\n';
		break;
	}

	return state;
}

INT check_network(std::vector <nInterface>& vInterfaces) 
{
	CONST WCHAR *perfIn = L"\\Network Interface(*)\\Bytes Received/sec";
	CONST WCHAR *perfOut = L"\\Network Interface(*)\\Bytes Sent/sec";

	PDH_HQUERY phQuery = NULL;
	PDH_HCOUNTER phCounterIn, phCounterOut;
	DWORD dwBufferSizeIn = 0, dwBufferSizeOut = 0, dwItemCount = 0;
	PDH_FMT_COUNTERVALUE_ITEM *pDisplayValuesIn = NULL, *pDisplayValuesOut = NULL;
	PDH_STATUS err;

	if (debug)
		std::wcout << L"Creating Query and adding counters" << '\n';

	err = PdhOpenQuery(NULL, NULL, &phQuery);
	if (!SUCCEEDED(err))
		goto die;

	err = PdhAddEnglishCounter(phQuery, perfIn, NULL, &phCounterIn);
	if (!SUCCEEDED(err)) 
		goto die;
	
	err = PdhAddEnglishCounter(phQuery, perfOut, NULL, &phCounterOut);
	if (!SUCCEEDED(err)) 
		goto die;
	
	if (debug)
		std::wcout << L"Collecting first batch of query data" << '\n';

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (debug)
		std::wcout << L"Sleep for one second" << '\n';

	Sleep(1000);

	if (debug)
		std::wcout << L"Collecting second batch of query data" << '\n';

	err = PdhCollectQueryData(phQuery);
	if (!SUCCEEDED(err))
		goto die;

	if (debug)
		std::wcout << L"Creating formatted counter arrays" << '\n';
	
	err = PdhGetFormattedCounterArray(phCounterIn, PDH_FMT_LONG, &dwBufferSizeIn, &dwItemCount, pDisplayValuesIn);
	if (err == PDH_MORE_DATA || SUCCEEDED(err))
		pDisplayValuesIn = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(new BYTE[dwItemCount*dwBufferSizeIn]);
	else
		goto die;
	
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

	if (debug)
		std::wcout << L"Going over counter array" << '\n';

	for (DWORD i = 0; i < dwItemCount; i++) {
		nInterface *iface = new nInterface(std::wstring(pDisplayValuesIn[i].szName));
		iface->BytesInSec = pDisplayValuesIn[i].FmtValue.longValue;
		iface->BytesOutSec = pDisplayValuesOut[i].FmtValue.longValue;
		vInterfaces.push_back(*iface);
		if (debug)
			std::wcout << L"Collected interface " << pDisplayValuesIn[i].szName << '\n';
	}
	if (debug)
		std::wcout << L"Finished collection. Cleaning up and returning" << '\n';

	if (phQuery)
		PdhCloseQuery(phQuery);
	if (pDisplayValuesIn)
		delete reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(pDisplayValuesIn);
	if (pDisplayValuesOut)
		delete reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(pDisplayValuesOut);
	return -1;
die:
	die(err);
	if (phQuery)
		PdhCloseQuery(phQuery);
	if (pDisplayValuesIn)
		delete reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(pDisplayValuesIn);
	if (pDisplayValuesOut)
		delete reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(pDisplayValuesOut);
	return 3;
}

BOOL mapSystemNamesToFamiliarNames(std::map<std::wstring, std::wstring>& mapNames)
{
	DWORD dwSize = 0, dwRetVal = 0;

	ULONG family = AF_UNSPEC, flags = GAA_FLAG_INCLUDE_PREFIX,
		outBufLen = 0, Iterations = 0;
	LPVOID lpMsgBuf = NULL;

	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
	/*
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
	PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
	PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
	PIP_ADAPTER_DNS_SERVER_ADDRESS pDnsServer = NULL;
	PIP_ADAPTER_PREFIX pPrefix = NULL;
	*/
	outBufLen = 15000; //15KB as suggestet by msdn of GetAdaptersAddresses

	if (debug)
		std::wcout << "Mapping adapter system names to friendly names\n";

	do {
		pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(new BYTE[outBufLen]);

		dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

		if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
			delete[]pAddresses;
			pAddresses = NULL;
		} else
			break;
	} while (++Iterations < 3);

	if (dwRetVal != NO_ERROR) {
		std::wcout << "Failed to collect friendly adapter names\n";
		delete[]pAddresses;
		return FALSE;
	}

	pCurrAddresses = pAddresses;
	std::wstringstream wssAdapterName;
	std::wstringstream wssFriendlyName;
	for (pCurrAddresses = pAddresses; pCurrAddresses; pCurrAddresses = pCurrAddresses->Next) {
		wssAdapterName.str(std::wstring());
		wssFriendlyName.str(std::wstring());
		wssAdapterName << pCurrAddresses->Description;
		wssFriendlyName << pCurrAddresses->FriendlyName;
		if (debug)
			std::wcout << "Got: " << wssAdapterName.str() << " -- " << wssFriendlyName.str() << '\n';

		mapNames.insert(std::pair<std::wstring, std::wstring>(wssAdapterName.str(), wssFriendlyName.str()));
	}

	delete[]pAddresses;
	return TRUE;
}
