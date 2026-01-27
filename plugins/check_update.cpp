// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <windows.h>
#include <shlwapi.h>
#include <wuapi.h>
#include <wuerror.h>

#define VERSION 1.0

#define CRITERIA L"(IsInstalled = 0 and CategoryIDs contains '0fa1201d-4330-4fa8-8ae9-b877473b6441') or (IsInstalled = 0 and CategoryIDs contains 'E6CF1350-C01B-414D-A61F-263D14D133B4')"

namespace po = boost::program_options;

struct printInfoStruct
{
	int warn{0};
	int crit{0};
	LONG numUpdates{0};
	bool ignoreReboot{false};
	int reboot{0};
	bool careForCanRequest{false};
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
		("debug,d", "Verbose/Debug output")
		("warning,w", po::value<int>(), "Number of updates to trigger a warning.")
		("critical,c", po::value<int>(), "Number of updates to trigger a critical.")
		("possible-reboot", "Treat \"update may need reboot\" as \"update needs reboot\"")
		("no-reboot-critical", "Do not automatically return critical if an update requiring reboot is present.")
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
			L"%s is a simple program to check a machines required updates.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nAfter some time, it will then output a string like this one:\n\n"
			L"\tUPDATE WARNING 8 | updates=8;1;1;0\n\n"
			L"\"UPDATE\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"8\" is the number of important updates.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value.\n\n"
			L"An update counts as important when it is part of the Security- or\n"
			L"CriticalUpdates group.\n"
			L"Consult the msdn on WSUS Classification GUIDs for more information.\n"
			L"%s' exit codes denote the following:\n"
			L" 0\tOK,\n\tNo Thresholds were broken or the programs check part was not executed\n"
			L" 1\tWARNING,\n\tThe warning, but not the critical threshold was broken\n"
			L" 2\tCRITICAL,\n\tThe critical threshold was broken or an update required reboot.\n"
			L" 3\tUNKNOWN, \n\tThe program experienced an internal or input error\n\n"
			L"If a warning threshold is set but not a critical threshold, the critical\n"
			L"threshold will be set to one greater than the set warning threshold.\n\n"
			L"The \"possible-reboot\" option is not recommended since this true for nearly\n"
			L"every update."
			, progName);
		std::cout << '\n';
		return 0;
	} if (vm.count("version")) {
		std::cout << "Version: " << VERSION << '\n';
		return 0;
	}
	if(vm.count("warning"))
		printInfo.warn = vm["warning"].as<int>();
	if (vm.count("critical"))
		printInfo.crit = vm["critical"].as<int>();
	else if (vm.count("warning"))
		printInfo.crit = printInfo.warn + 1;
	printInfo.careForCanRequest = vm.count("possible-reboot") > 0;
	printInfo.ignoreReboot = vm.count("no-reboot-critical") > 0;

	l_Debug = vm.count("debug") > 0;

	return -1;
}

static int printOutput(const printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Constructing output string" << '\n';

	state state = OK;
	std::wstring output = L"UPDATE ";

	if (printInfo.numUpdates >= printInfo.warn && printInfo.warn)
		state = WARNING;

	if ((printInfo.reboot && !printInfo.ignoreReboot) || (printInfo.numUpdates >= printInfo.crit && printInfo.crit))
		state = CRITICAL;

	switch (state) {
	case OK:
		output.append(L"OK ");
		break;
	case WARNING:
		output.append(L"WARNING ");
		break;
	case CRITICAL:
		output.append(L"CRITICAL ");
		break;
	}
	output.append(std::to_wstring(printInfo.numUpdates));
	if (printInfo.reboot) {
		output.append(L"; ");
		output.append(std::to_wstring(printInfo.reboot));
		output.append(L" NEED REBOOT ");
	}
	std::wcout << output << L" | 'update'=" << printInfo.numUpdates << L";"
		<< printInfo.warn << L";" << printInfo.crit << L";0;" << '\n';

	return state;
}

static int check_update(printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << "Initializing COM library" << '\n';
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	ISearchResult *pResult;
	IUpdateSession *pSession;
	IUpdateSearcher *pSearcher;
	BSTR criteria = NULL;

	HRESULT err;
	if (l_Debug)
		std::wcout << "Creating UpdateSession and UpdateSearcher" << '\n';
	CoCreateInstance(CLSID_UpdateSession, NULL, CLSCTX_INPROC_SERVER, IID_IUpdateSession, (void **)&pSession);
	pSession->CreateUpdateSearcher(&pSearcher);

	/*
	* IsInstalled = 0: All updates, including languagepacks and features
	* BrowseOnly = 0: No features or languagepacks, security and unnamed
	* BrowseOnly = 1: Nothing, broken
	* RebootRequired = 1: Reboot required
	*/

	criteria = SysAllocString(CRITERIA);
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa386526%28v=vs.85%29.aspx
	// https://msdn.microsoft.com/en-us/library/ff357803%28v=vs.85%29.aspx

	if (l_Debug)
		std::wcout << L"Querying updates from server" << '\n';

	err = pSearcher->Search(criteria, &pResult);
	if (!SUCCEEDED(err))
		goto die;
	SysFreeString(criteria);

	IUpdateCollection *pCollection;
	IUpdate *pUpdate;

	LONG updateSize;
	pResult->get_Updates(&pCollection);
	pCollection->get_Count(&updateSize);

	if (updateSize == 0)
		return -1;

	printInfo.numUpdates = updateSize;
	//	printInfo.important = printInfo.warn;

	IInstallationBehavior *pIbehav;
	InstallationRebootBehavior updateReboot;

	for (LONG i = 0; i < updateSize; i++) {
		pCollection->get_Item(i, &pUpdate);
		if (l_Debug) {
			std::wcout << L"Checking reboot behaviour of update number " << i << '\n';
		}
		pUpdate->get_InstallationBehavior(&pIbehav);
		pIbehav->get_RebootBehavior(&updateReboot);
		if (updateReboot == irbAlwaysRequiresReboot) {
			printInfo.reboot++;
			if (l_Debug)
				std::wcout << L"It requires reboot" << '\n';
			continue;
		}
		if (printInfo.careForCanRequest && updateReboot == irbCanRequestReboot) {
			if (l_Debug)
				std::wcout << L"It requires reboot" << '\n';
			printInfo.reboot++;
		}
	}

	if (l_Debug)
		std::wcout << L"Cleaning up and returning" << '\n';

	SysFreeString(criteria);
	CoUninitialize();
	return -1;

die:
	printErrorInfo(err);
	CoUninitialize();
	if (criteria)
		SysFreeString(criteria);
	return 3;
}

int wmain(int argc, WCHAR **argv)
{
	printInfoStruct printInfo;
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_update(printInfo);
	if (ret != -1)
		return ret;

	return printOutput(printInfo);
}
