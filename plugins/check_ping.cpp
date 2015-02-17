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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //else winsock will be included with windows.h and conflict with winsock2
#endif 

#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <Shlwapi.h>
#include <ws2ipdef.h>
#include <Mstcpip.h>
#include <Ws2tcpip.h>

#include <iostream>

#include "thresholds.h"
#include "boost/program_options.hpp"

#define VERSION 1.0

namespace po = boost::program_options;
using std::cout; using std::endl; using std::wcout;
using std::wstring; using std::string;

static BOOL debug = FALSE;

struct response
{
	double avg;
	UINT pMin = 0, pMax = 0, dropped = 0;
};

struct printInfoStruct
{
	threshold warn, crit;
	threshold wpl, cpl;
	wstring host;
	BOOL ipv4 = TRUE;
	DWORD timeout = 1000;
	int num = 5;
};

static int printOutput(printInfoStruct&, response&);
static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int check_ping4(const printInfoStruct&, response&);
static int check_ping6(const printInfoStruct&, response&);

int wmain(int argc, wchar_t **argv)
{
	po::variables_map vm;
	printInfoStruct printInfo;
	response response;

	WSADATA dat;
	WORD req = MAKEWORD(1, 1);

	WSAStartup(req, &dat);

	if (parseArguments(argc, argv, vm, printInfo) != -1)
		return 3;
	if (printInfo.ipv4) {
		if (check_ping4(printInfo, response) != -1)
			return 3;
	} else {
		if (check_ping6(printInfo, response) != -1)
			return 3;
	}

	WSACleanup();
	return printOutput(printInfo, response);
}

int parseArguments(int ac, wchar_t **av, po::variables_map& vm, printInfoStruct& printInfo)
{
	wchar_t namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	wchar_t *progName = PathFindFileName(namePath);

	po::options_description desc;

	desc.add_options()
		("help,h", "print usage message and exit")
		("version,V", "print version and exit")
		("debug,d", "Verbose/Debug output")
		("host,H", po::wvalue<wstring>()->required(), "host ip to ping")
		(",4", "--host is an ipv4 address (default)")
		(",6", "--host is an ipv6 address")
		("timeout,T", po::value<int>(), "specify timeout for requests in ms (default=1000)")
		("ncount,n", po::value<int>(), "declare ping count (default=5)")
		("warning,w", po::wvalue<wstring>(), "warning values: rtt,package loss")
		("critical,c", po::wvalue<wstring>(), "critical values: rtt,package loss")
		;

	po::basic_command_line_parser<wchar_t> parser(ac, av);

	try {
		po::store(
			parser
			.options(desc)
			.style(
			po::command_line_style::unix_style |
			po::command_line_style::allow_long_disguise &
			~po::command_line_style::allow_guessing
			)
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
			L"%s is a simple program to ping an ip4 address.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will take at least timeout times number of pings to run\n"
			L"Then it will output a string looking something like this:\n\n"
			L"\tPING WARNING RTA: 72ms Packet loss: 20% | ping=72ms;40;80;71;77 pl=20%;20;50;0;100\n\n"
			L"\"PING4\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"RTA: 72ms Packet loss: 20%\" the relevant information.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. \n\n"
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
		cout << progName << " Version: " << VERSION << endl;
		return 0;
	}

	if (vm.count("-4") && vm.count("-6")) {
		cout << "Conflicting options \"4\" and \"6\"" << endl;
		return 3;
	}
    
	if (vm.count("warning")) {
		std::vector<wstring> sVec = splitMultiOptions(vm["warning"].as<wstring>());
		if (sVec.size() != 2) {
			cout << "Wrong format for warning thresholds" << endl;
			return 3;
		}
		try {
			printInfo.warn = threshold(*sVec.begin());
			printInfo.wpl = threshold(sVec.back());
			if (!printInfo.wpl.perc) {
				cout << "Packet loss must be percentage" << endl;
				return 3;
			}
		} catch (std::invalid_argument& e) {
			cout << e.what() << endl;
			return 3;
		}
	}
	if (vm.count("critical")) {
		std::vector<wstring> sVec = splitMultiOptions(vm["critical"].as<wstring>());
		if (sVec.size() != 2) {
			cout << "Wrong format for critical thresholds" << endl;
			return 3;
		}
		try {
			printInfo.crit = threshold(*sVec.begin());
			printInfo.cpl = threshold(sVec.back());
			if (!printInfo.wpl.perc) {
				cout << "Packet loss must be percentage" << endl;
				return 3;
			}
		} catch (std::invalid_argument& e) {
			cout << e.what() << endl;
			return 3;
		}
	}

	if (vm.count("timeout"))
		printInfo.timeout = vm["timeout"].as<int>();
	if (vm.count("count"))
		printInfo.num = vm["count"].as<int>();
	if (vm.count("-6"))
		printInfo.ipv4 = FALSE;

	printInfo.host = vm["host"].as<wstring>();

	if (vm.count("debug"))
		debug = TRUE;

	return -1;
}

int printOutput(printInfoStruct& printInfo, response& response)
{
	if (debug)
		wcout << L"Constructing output string" << endl;

	state state = OK;

	double plp = ((double)response.dropped / printInfo.num) * 100.0;

	if (printInfo.warn.rend(response.avg) || printInfo.wpl.rend(plp))
		state = WARNING;

	if (printInfo.crit.rend(response.avg) || printInfo.cpl.rend(plp))
		state = CRITICAL;

	std::wstringstream perf;
	perf << L"rta=" << response.avg << L"ms;" << printInfo.warn.pString() << L";"
		<< printInfo.crit.pString() << L";0;" << " pl=" << removeZero(plp) << "%;" 
		<< printInfo.wpl.pString() << ";" << printInfo.cpl.pString() << ";0;100";

	if (response.dropped == printInfo.num) {
		wcout << L"PING CRITICAL ALL CONNECTIONS DROPPED | " << perf.str() << endl;
		return 3;
	}

	switch (state) {
	case OK:
		wcout << L"PING OK RTA: " << response.avg << L"ms Packet loss: " << removeZero(plp) << "% | " << perf.str() << endl;
		break;
	case WARNING:
		wcout << L"PING WARNING RTA: " << response.avg << L"ms Packet loss: " << removeZero(plp) << "% | " << perf.str() << endl;
		break;
	case CRITICAL:
		wcout << L"PING CRITICAL RTA: " << response.avg << L"ms Packet loss: " << removeZero(plp) << "% | " << perf.str() << endl;
		break;
	}

	return state;
}

int check_ping4(const printInfoStruct& pi, response& response)
{
	in_addr ipDest4;
	HANDLE hIcmp;
	DWORD dwRet = 0, dwRepSize = 0;
	LPVOID repBuf = NULL;
	UINT rtt = 0;
	int num = pi.num;
	LARGE_INTEGER frequency, timer1, timer2;
	LPCWSTR term;

	if (debug)
		wcout << L"Parsing ip address" << endl;

	if (RtlIpv4StringToAddress(pi.host.c_str(), TRUE, &term, &ipDest4) == STATUS_INVALID_PARAMETER) {
		std::wcout << pi.host << " is not a valid ip address" << std::endl;
		return 3;
	}

	if (*term != L'\0') {
		std::wcout << pi.host << " is not a valid ip address" << std::endl;
		return 3;
	}

	if (debug)
		wcout << L"Creating Icmp File" << endl;

	if ((hIcmp = IcmpCreateFile()) == INVALID_HANDLE_VALUE)
		goto die;

	dwRepSize = sizeof(ICMP_ECHO_REPLY) + 8;
	repBuf = reinterpret_cast<VOID *>(new BYTE[dwRepSize]);

	if (repBuf == NULL)
		goto die;

	QueryPerformanceFrequency(&frequency);
	do {
		QueryPerformanceCounter(&timer1);

		if (debug)
			wcout << L"Sending Icmp echo" << endl;

		if (!IcmpSendEcho2(hIcmp, NULL, NULL, NULL, ipDest4.S_un.S_addr,
			NULL, 0, NULL, repBuf, dwRepSize, pi.timeout)) {
			response.dropped++;
			if (debug)
				wcout << L"Dropped: Response was 0" << endl;
			continue;
		}

		if (debug)
			wcout << "Ping recieved" << endl;

		PICMP_ECHO_REPLY pEchoReply = static_cast<PICMP_ECHO_REPLY>(repBuf);

		if (pEchoReply->Status != IP_SUCCESS) {
			response.dropped++;
			if (debug)
				wcout << L"Dropped: echo reply status " << pEchoReply->Status << endl;
			continue;
		}

		if (debug)
			wcout << L"Recorded rtt of " << pEchoReply->RoundTripTime << endl;

		rtt += pEchoReply->RoundTripTime;
		if (response.pMin == 0 || pEchoReply->RoundTripTime < response.pMin)
			response.pMin = pEchoReply->RoundTripTime;
		else if (pEchoReply->RoundTripTime > response.pMax)
			response.pMax = pEchoReply->RoundTripTime;

		QueryPerformanceCounter(&timer2);
		if (((timer2.QuadPart - timer1.QuadPart) * 1000 / frequency.QuadPart) < pi.timeout)
			Sleep(pi.timeout - ((timer2.QuadPart - timer1.QuadPart) * 1000 / frequency.QuadPart));
	} while (--num);

	if (debug)
		wcout << L"All pings sent. Cleaning up and returning" << endl;

	if (hIcmp)
		IcmpCloseHandle(hIcmp);
	if (repBuf)
		delete reinterpret_cast<VOID *>(repBuf);

	response.avg = ((double)rtt / pi.num);

	return -1;

die:
	die();
	if (hIcmp)
		IcmpCloseHandle(hIcmp);
	if (repBuf)
		delete reinterpret_cast<VOID *>(repBuf);

	return 3;
}

int check_ping6(const printInfoStruct& pi, response& response)
{
	PCWSTR term;
	sockaddr_in6 ipDest6, ipSource6;
	IP_OPTION_INFORMATION ipInfo = { 30, 0, 0, 0, NULL };
	DWORD dwRepSize = sizeof(ICMPV6_ECHO_REPLY) + 8;
	LPVOID repBuf = reinterpret_cast<VOID *>(new BYTE[dwRepSize]);
	HANDLE hIcmp = NULL;

	LARGE_INTEGER frequency, timer1, timer2;
	int num = pi.num;
	UINT rtt = 0;

	if (debug)
		wcout << L"Parsing ip address" << endl;

	if (RtlIpv6StringToAddressEx(pi.host.c_str(), &ipDest6.sin6_addr, &ipDest6.sin6_scope_id, &ipDest6.sin6_port)) {
		std::wcout << pi.host << " is not a valid ipv6 address" << std::endl;
		return 3;
	}

	ipDest6.sin6_family = AF_INET6;

	ipSource6.sin6_addr = in6addr_any;
	ipSource6.sin6_family = AF_INET6;
	ipSource6.sin6_flowinfo = 0;
	ipSource6.sin6_port = 0;

	if (debug)
		wcout << L"Creating Icmp File" << endl;

	hIcmp = Icmp6CreateFile();
	if (hIcmp == INVALID_HANDLE_VALUE) {
		goto die;
	}

	QueryPerformanceFrequency(&frequency);
	do {
		QueryPerformanceCounter(&timer1);

		if (debug)
			wcout << L"Sending Icmp echo" << endl;

		if (!Icmp6SendEcho2(hIcmp, NULL, NULL, NULL, &ipSource6, &ipDest6,
			NULL, 0, &ipInfo, repBuf, dwRepSize, pi.timeout)) {
			response.dropped++;
			if (debug)
				wcout << L"Dropped: Response was 0" << endl;
			continue;
		}

		if (debug)
			wcout << "Ping recieved" << endl;

		Icmp6ParseReplies(repBuf, dwRepSize);

		ICMPV6_ECHO_REPLY *pEchoReply = static_cast<ICMPV6_ECHO_REPLY *>(repBuf);

		if (pEchoReply->Status != IP_SUCCESS) {
			response.dropped++;
			if (debug)
				wcout << L"Dropped: echo reply status " << pEchoReply->Status << endl;
			continue;
		}
		
		rtt += pEchoReply->RoundTripTime;

		if (debug)
			wcout << L"Recorded rtt of " << pEchoReply->RoundTripTime << endl;

		if (response.pMin == 0 || pEchoReply->RoundTripTime < response.pMin)
			response.pMin = pEchoReply->RoundTripTime;
		else if (pEchoReply->RoundTripTime > response.pMax)
			response.pMax = pEchoReply->RoundTripTime;

		QueryPerformanceCounter(&timer2);
		if (((timer2.QuadPart - timer1.QuadPart) * 1000 / frequency.QuadPart) < pi.timeout)
			Sleep(pi.timeout - ((timer2.QuadPart - timer1.QuadPart) * 1000 / frequency.QuadPart));
	} while (--num);

	if (debug)
		wcout << L"All pings sent. Cleaning up and returning" << endl;

	if (hIcmp)
		IcmpCloseHandle(hIcmp);
	if (repBuf)
		delete reinterpret_cast<VOID *>(repBuf);
	response.avg = ((double)rtt / pi.num);

	return -1;
die:
	die(GetLastError());

	if (hIcmp)
		IcmpCloseHandle(hIcmp);
	if (repBuf)
		delete reinterpret_cast<VOID *>(repBuf);

	return 3;
}