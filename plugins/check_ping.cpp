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

#include "check_ping.h"

#define VERSION 1.0

namespace po = boost::program_options;

static BOOL debug = FALSE;

INT wmain(INT argc, WCHAR **argv)
{
	po::variables_map vm;
	printInfoStruct printInfo;
	response response;

	WSADATA dat;

	if (WSAStartup(MAKEWORD(2, 2), &dat)) {
		std::cout << "WSAStartup failed\n";
		return 3;
	}

	if (parseArguments(argc, argv, vm, printInfo) != -1)
		return 3;

	if (!resolveHostname(printInfo.host, printInfo.ipv6, printInfo.ip))
		return 3;

	if (printInfo.ipv6) {
		if (check_ping6(printInfo, response) != -1)
			return 3;
	} else {
		if (check_ping4(printInfo, response) != -1)
			return 3;
	}

	WSACleanup();
	return printOutput(printInfo, response);
}

INT parseArguments(INT ac, WCHAR **av, po::variables_map& vm, printInfoStruct& printInfo)
{
	WCHAR namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	WCHAR *progName = PathFindFileName(namePath);

	po::options_description desc;

	desc.add_options()
		("help,h", "Print usage message and exit")
		("version,V", "Print version and exit")
		("debug,d", "Verbose/Debug output")
		("host,H", po::wvalue<std::wstring>()->required(), "Target hostname or IP. If an IPv6 address is given, the '-6' option must be set")
		(",4", "--Host is an IPv4 address or if it's a hostname: Resolve it to an IPv4 address (default)")
		(",6", "--Host is an IPv6 address or if it's a hostname: Resolve it to an IPv6 address")
		("timeout,t", po::value<INT>(), "Specify timeout for requests in ms (default=1000)")
		("packets,p", po::value<INT>(), "Declare ping count (default=5)")
		("warning,w", po::wvalue<std::wstring>(), "Warning values: rtt,package loss")
		("critical,c", po::wvalue<std::wstring>(), "Critical values: rtt,package loss")
		;

	po::basic_command_line_parser<WCHAR> parser(ac, av);

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
		std::cout << e.what() << '\n' << desc << '\n';
		return 3;
	}

	if (vm.count("help")) {
		std::wcout << progName << " Help\n\tVersion: " << VERSION << '\n';
		wprintf(
			L"%s is a simple program to ping an ip4 address.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nIt will take at least timeout times number of pings to run\n"
			L"Then it will output a string looking something like this:\n\n"
			L"\tPING WARNING RTA: 72ms Packet loss: 20% | ping=72ms;40;80;71;77 pl=20%;20;50;0;100\n\n"
			L"\"PING\" being the type of the check, \"WARNING\" the returned status\n"
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
		std::cout << '\n';
		return 0;
	}

	if (vm.count("version")) {
		std::cout << progName << " Version: " << VERSION << '\n';
		return 0;
	}

	if (vm.count("-4") && vm.count("-6")) {
		std::cout << "Conflicting options \"4\" and \"6\"" << '\n';
		return 3;
	}
	if (vm.count("-6"))
		printInfo.ipv6 = TRUE;

	if (vm.count("warning")) {
		std::vector<std::wstring> sVec = splitMultiOptions(vm["warning"].as<std::wstring>());
		if (sVec.size() != 2) {
			std::cout << "Wrong format for warning thresholds" << '\n';
			return 3;
		}
		try {
			printInfo.warn = threshold(*sVec.begin());
			printInfo.wpl = threshold(sVec.back());
			if (!printInfo.wpl.perc) {
				std::cout << "Packet loss must be percentage" << '\n';
				return 3;
			}
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}
	if (vm.count("critical")) {
		std::vector<std::wstring> sVec = splitMultiOptions(vm["critical"].as<std::wstring>());
		if (sVec.size() != 2) {
			std::cout << "Wrong format for critical thresholds" << '\n';
			return 3;
		}
		try {
			printInfo.crit = threshold(*sVec.begin());
			printInfo.cpl = threshold(sVec.back());
			if (!printInfo.wpl.perc) {
				std::cout << "Packet loss must be percentage" << '\n';
				return 3;
			}
		} catch (std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}

	if (vm.count("timeout"))
		printInfo.timeout = vm["timeout"].as<INT>();
	if (vm.count("packets"))
		printInfo.num = vm["packets"].as<INT>();


	printInfo.host = vm["host"].as<std::wstring>();

	if (vm.count("debug"))
		debug = TRUE;

	return -1;
}

INT printOutput(printInfoStruct& printInfo, response& response)
{
	if (debug)
		std::wcout << L"Constructing output string" << '\n';

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
		std::wcout << L"PING CRITICAL ALL CONNECTIONS DROPPED | " << perf.str() << '\n';
		return 3;
	}

	switch (state) {
	case OK:
		std::wcout << L"PING OK RTA: " << response.avg << L"ms Packet loss: " << removeZero(plp) << "% | " << perf.str() << '\n';
		break;
	case WARNING:
		std::wcout << L"PING WARNING RTA: " << response.avg << L"ms Packet loss: " << removeZero(plp) << "% | " << perf.str() << '\n';
		break;
	case CRITICAL:
		std::wcout << L"PING CRITICAL RTA: " << response.avg << L"ms Packet loss: " << removeZero(plp) << "% | " << perf.str() << '\n';
		break;
	}

	return state;
}

BOOL resolveHostname(CONST std::wstring hostname, BOOL ipv6, std::wstring& ipaddr)
{
	ADDRINFOW *result = NULL;
	ADDRINFOW *ptr = NULL;
	ADDRINFOW hints;
	ZeroMemory(&hints, sizeof(hints));
	wchar_t ipstringbuffer[46];

	if (ipv6)
		hints.ai_family = AF_INET6;
	else
		hints.ai_family = AF_INET;

	if (debug)
		std::wcout << L"Resolving hostname \"" << hostname << L"\"\n";

	DWORD ret = GetAddrInfoW(hostname.c_str(), NULL, &hints, &result);

	if (ret) {
		std::cout << "Failed to resolve hostname. Winsock Error Code: " << ret << '\n';
		return false;
	}

	if (ipv6) {
		struct sockaddr_in6 *address6 = (struct sockaddr_in6 *) result->ai_addr;
		InetNtop(AF_INET6, &address6->sin6_addr, ipstringbuffer, 46);
	} else {
		struct sockaddr_in *address4 = (struct sockaddr_in *) result->ai_addr;
		InetNtop(AF_INET, &address4->sin_addr, ipstringbuffer, 46);
	}

	if (debug)
		std::wcout << L"Resolved to \"" << ipstringbuffer << L"\"\n";

	ipaddr = ipstringbuffer;
	return true;
}

INT check_ping4(CONST printInfoStruct& pi, response& response)
{
	in_addr ipDest4;
	HANDLE hIcmp;
	DWORD dwRet = 0, dwRepSize = 0;
	LPVOID repBuf = NULL;
	UINT rtt = 0;
	INT num = pi.num;
	LARGE_INTEGER frequency, timer1, timer2;
	LPCWSTR term;

	if (debug)
		std::wcout << L"Parsing ip address" << '\n';

	if (RtlIpv4StringToAddress(pi.ip.c_str(), TRUE, &term, &ipDest4) == STATUS_INVALID_PARAMETER) {
		std::wcout << pi.ip << " is not a valid ip address\n";
		return 3;
	}

	if (*term != L'\0') {
		std::wcout << pi.ip << " is not a valid ip address\n";
		return 3;
	}

	if (debug)
		std::wcout << L"Creating Icmp File\n";

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
			std::wcout << L"Sending Icmp echo\n";

		if (!IcmpSendEcho2(hIcmp, NULL, NULL, NULL, ipDest4.S_un.S_addr,
			NULL, 0, NULL, repBuf, dwRepSize, pi.timeout)) {
			response.dropped++;
			if (debug)
				std::wcout << L"Dropped: Response was 0" << '\n';
			continue;
		}

		if (debug)
			std::wcout << "Ping recieved" << '\n';

		PICMP_ECHO_REPLY pEchoReply = static_cast<PICMP_ECHO_REPLY>(repBuf);

		if (pEchoReply->Status != IP_SUCCESS) {
			response.dropped++;
			if (debug)
				std::wcout << L"Dropped: echo reply status " << pEchoReply->Status << '\n';
			continue;
		}

		if (debug)
			std::wcout << L"Recorded rtt of " << pEchoReply->RoundTripTime << '\n';

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
		std::wcout << L"All pings sent. Cleaning up and returning" << '\n';

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

INT check_ping6(CONST printInfoStruct& pi, response& response)
{
	sockaddr_in6 ipDest6, ipSource6;
	IP_OPTION_INFORMATION ipInfo = { 30, 0, 0, 0, NULL };
	DWORD dwRepSize = sizeof(ICMPV6_ECHO_REPLY) + 8;
	LPVOID repBuf = reinterpret_cast<VOID *>(new BYTE[dwRepSize]);
	HANDLE hIcmp = NULL;

	LARGE_INTEGER frequency, timer1, timer2;
	INT num = pi.num;
	UINT rtt = 0;

	if (debug)
		std::wcout << L"Parsing ip address" << '\n';

	if (RtlIpv6StringToAddressEx(pi.ip.c_str(), &ipDest6.sin6_addr, &ipDest6.sin6_scope_id, &ipDest6.sin6_port)) {
		std::wcout << pi.ip << " is not a valid ipv6 address" << '\n';
		return 3;
	}

	ipDest6.sin6_family = AF_INET6;

	ipSource6.sin6_addr = in6addr_any;
	ipSource6.sin6_family = AF_INET6;
	ipSource6.sin6_flowinfo = 0;
	ipSource6.sin6_port = 0;

	if (debug)
		std::wcout << L"Creating Icmp File" << '\n';

	hIcmp = Icmp6CreateFile();
	if (hIcmp == INVALID_HANDLE_VALUE) {
		goto die;
	}

	QueryPerformanceFrequency(&frequency);
	do {
		QueryPerformanceCounter(&timer1);

		if (debug)
			std::wcout << L"Sending Icmp echo" << '\n';

		if (!Icmp6SendEcho2(hIcmp, NULL, NULL, NULL, &ipSource6, &ipDest6,
			NULL, 0, &ipInfo, repBuf, dwRepSize, pi.timeout)) {
			response.dropped++;
			if (debug)
				std::wcout << L"Dropped: Response was 0" << '\n';
			continue;
		}

		if (debug)
			std::wcout << "Ping recieved" << '\n';

		Icmp6ParseReplies(repBuf, dwRepSize);

		ICMPV6_ECHO_REPLY *pEchoReply = static_cast<ICMPV6_ECHO_REPLY *>(repBuf);

		if (pEchoReply->Status != IP_SUCCESS) {
			response.dropped++;
			if (debug)
				std::wcout << L"Dropped: echo reply status " << pEchoReply->Status << '\n';
			continue;
		}
		
		rtt += pEchoReply->RoundTripTime;

		if (debug)
			std::wcout << L"Recorded rtt of " << pEchoReply->RoundTripTime << '\n';

		if (response.pMin == 0 || pEchoReply->RoundTripTime < response.pMin)
			response.pMin = pEchoReply->RoundTripTime;
		else if (pEchoReply->RoundTripTime > response.pMax)
			response.pMax = pEchoReply->RoundTripTime;

		QueryPerformanceCounter(&timer2);
		if (((timer2.QuadPart - timer1.QuadPart) * 1000 / frequency.QuadPart) < pi.timeout)
			Sleep(pi.timeout - ((timer2.QuadPart - timer1.QuadPart) * 1000 / frequency.QuadPart));
	} while (--num);

	if (debug)
		std::wcout << L"All pings sent. Cleaning up and returning" << '\n';

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
