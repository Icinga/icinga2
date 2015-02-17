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
#include <windows.h>
#include <Shlwapi.h>
#include <iostream>
#include <wuapi.h>
#include <wuerror.h>

#include "thresholds.h"

#include "boost/program_options.hpp"

#define VERSION 1.0

#define CRITERIA L"(IsInstalled = 0 and CategoryIDs contains '0fa1201d-4330-4fa8-8ae9-b877473b6441') or (IsInstalled = 0 and CategoryIDs contains 'E6CF1350-C01B-414D-A61F-263D14D133B4')"

namespace po = boost::program_options;

using std::wcout; using std::endl;
using std::wstring; using std::cout;

static BOOL debug = FALSE;

struct printInfoStruct 
{
	BOOL warn, crit;
	LONG numUpdates;
	BOOL important, reboot, careForCanRequest;
};

static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(const printInfoStruct&);
static int check_update(printInfoStruct&);

int wmain(int argc, wchar_t **argv) 
{
	printInfoStruct printInfo = { FALSE, FALSE, 0, FALSE, FALSE, FALSE };
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;
	
	ret = check_update(printInfo);
	if (ret != -1)
		return ret;

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
		("warning,w", "warn if there are important updates available")
		("critical,c", "critical if there are important updates that require a reboot")
		("possible-reboot", "treat \"update may need reboot\" as \"update needs reboot\"")
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
			L"%s is a simple program to check a machines required updates.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nAfter some time, it will then output a string like this one:\n\n"
			L"\tUPDATE WARNING 8 | updates=8;1;1;0\n\n"
			L"\"UPDATE\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"8\" is the number of important updates updates.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. Performance data will only be displayed when\n"
			L"you set at least one threshold\n\n"
			L"An update counts as important when it is part of the Security- or\n"
			L"CriticalUpdates group.\n"
			L"Consult the msdn on WSUS Classification GUIDs for more information.\n"
			L"%s' exit codes denote the following:\n"
			L" 0\tOK,\n\tNo Thresholds were broken or the programs check part was not executed\n"
			L" 1\tWARNING,\n\tThe warning, but not the critical threshold was broken\n"
			L" 2\tCRITICAL,\n\tThe critical threshold was broken\n"
			L" 3\tUNKNOWN, \n\tThe program experienced an internal or input error\n\n"
			L"%s works different from other plugins in that you do not set thresholds\n"
			L"but only activate them. Using \"-w\" triggers warning state if there are not\n"
			L"installed and non-optional updates. \"-c\" triggers critical if there are\n"
			L"non-optional updates that require a reboot.\n"
			L"The \"possible-reboot\" option is not recommended since this true for nearly\n"
			L"every update."
			, progName, progName);
		cout << endl;
		return 0;
	} if (vm.count("version")) {
		cout << "Version: " << VERSION << endl;
		return 0;
	}

	if (vm.count("warning"))
		printInfo.warn = TRUE;

	if (vm.count("critical"))
		printInfo.crit = TRUE;

	if (vm.count("possible-reboot"))
		printInfo.careForCanRequest = TRUE;

	if (vm.count("debug"))
		debug = TRUE;

	return -1;
}

int printOutput(const printInfoStruct& printInfo)
{
	if (debug)
		wcout << L"Constructing output string" << endl;

	state state = OK;
	wstring output = L"UPDATE ";

	if (printInfo.important)
		state = WARNING;

	if (printInfo.reboot)
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

	wcout << output << printInfo.numUpdates << L" | update=" << printInfo.numUpdates << L";"
		<< printInfo.warn << L";" << printInfo.crit << L";0;" << endl;

	return state;
}

int check_update(printInfoStruct& printInfo) 
{
	if (debug)
		wcout << "Initializing COM library" << endl;
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	ISearchResult *pResult;
	IUpdateSession *pSession;
	IUpdateSearcher *pSearcher;
	BSTR criteria = NULL;

	HRESULT err;
	if (debug)
		wcout << "Creating UpdateSession and UpdateSearcher" << endl;
	CoCreateInstance(CLSID_UpdateSession, NULL, CLSCTX_INPROC_SERVER, IID_IUpdateSession, (LPVOID*)&pSession);
	pSession->CreateUpdateSearcher(&pSearcher);

	/*
	 IsInstalled = 0: All updates, including languagepacks and features
	 BrowseOnly = 0: No features or languagepacks, security and unnamed
	 BrowseOnly = 1: Nothing, broken
	 RebootRequired = 1: Reboot required
	*/

	criteria = SysAllocString(CRITERIA);
	// http://msdn.microsoft.com/en-us/library/windows/desktop/aa386526%28v=vs.85%29.aspx
	// http://msdn.microsoft.com/en-us/library/ff357803%28v=vs.85%29.aspx

	if (debug)
		wcout << L"Querrying updates from server" << endl;

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
		if (debug) {
			wcout << L"Checking reboot behaviour of update number " << i << endl;
		}
		pUpdate->get_InstallationBehavior(&pIbehav);
		pIbehav->get_RebootBehavior(&updateReboot);
		if (updateReboot == irbAlwaysRequiresReboot) {
			printInfo.reboot = TRUE;
			if (debug)
				wcout << L"It requires reboot" << endl;
			continue;
		}
		if (printInfo.careForCanRequest && updateReboot == irbCanRequestReboot)
			if (debug)
				wcout << L"It requires reboot" << endl;
			printInfo.reboot = TRUE;
	}

	if (debug)
		wcout << L"Cleaning up and returning" << endl;

	SysFreeString(criteria);
	CoUninitialize();
	return -1;

die:
	die(err);
	CoUninitialize();
	if (criteria)
		SysFreeString(criteria);
	return 3;
}