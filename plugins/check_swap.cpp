/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "plugins/thresholds.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <shlwapi.h>
#include <Psapi.h>
#include <vector>

#define VERSION 1.0

namespace po = boost::program_options;

struct printInfoStruct
{
	threshold warn;
	threshold crit;
	double tSwap;
	double aSwap;
	double percentFree;
	Bunit unit = BunitMB;
	bool showUsed;
};

struct pageFileInfo
{
	SIZE_T totalSwap;
	SIZE_T availableSpwap;
};

static bool l_Debug;

BOOL EnumPageFilesProc(LPVOID pContext, PENUM_PAGE_FILE_INFORMATION pPageFileInfo, LPCWSTR lpFilename) {
	std::vector<pageFileInfo>* pageFile = static_cast<std::vector<pageFileInfo>*>(pContext);
	SYSTEM_INFO systemInfo;

	GetSystemInfo(&systemInfo);

	// pPageFileInfo output is in pages, we need to multiply it by the page size
	pageFile->push_back({ pPageFileInfo->TotalSize * systemInfo.dwPageSize, (pPageFileInfo->TotalSize - pPageFileInfo->TotalInUse) * systemInfo.dwPageSize });

	return TRUE;
}

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
		("warning,w", po::wvalue<std::wstring>(), "Warning threshold")
		("critical,c", po::wvalue<std::wstring>(), "Critical threshold")
		("unit,u", po::wvalue<std::wstring>(), "The unit to use for display (default MB)")
		("show-used,U", "Show used swap instead of the free swap")
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
			L"%s is a simple program to check a machines swap in percent.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		std::cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tSWAP WARNING - 20%% free | swap=2000B;3000;500;0;10000\n\n"
			L"\"SWAP\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"20%%\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. Performance data will only be displayed when\n"
			L"you set at least one threshold\n\n"
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
			L"All of these options work with the critical threshold \"-c\" too.\n"
			, progName);
		std::cout << '\n';
		return 0;
	}

	if (vm.count("version"))
		std::wcout << L"Version: " << VERSION << '\n';

	if (vm.count("warning")) {
		try {
			printInfo.warn = threshold(vm["warning"].as<std::wstring>());
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
		printInfo.warn.legal = !printInfo.warn.legal;
	}

	if (vm.count("critical")) {
		try {
			printInfo.crit = threshold(vm["critical"].as<std::wstring>());
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
		printInfo.crit.legal = !printInfo.crit.legal;
	}

	l_Debug = vm.count("debug") > 0;

	if (vm.count("unit")) {
		try {
			printInfo.unit = parseBUnit(vm["unit"].as<std::wstring>());
		} catch (const std::invalid_argument& e) {
			std::cout << e.what() << '\n';
			return 3;
		}
	}

	if (vm.count("show-used")) {
		printInfo.showUsed = true;
		printInfo.warn.legal = true;
		printInfo.crit.legal = true;
	}

	return -1;
}

static int printOutput(printInfoStruct& printInfo)
{
	if (l_Debug)
		std::wcout << L"Constructing output string" << '\n';

	state state = OK;

	std::wcout << L"SWAP ";

	double currentValue;

	if (!printInfo.showUsed)
		currentValue = printInfo.aSwap;
	else
		currentValue = printInfo.tSwap - printInfo.aSwap;

	if (printInfo.warn.rend(currentValue, printInfo.tSwap))
		state = WARNING;

	if (printInfo.crit.rend(currentValue, printInfo.tSwap))
		state = CRITICAL;

	std::wcout << stateToString(state) << " - ";

	if (!printInfo.showUsed)
		std::wcout << printInfo.percentFree << L"% free ";
	else
		std::wcout << 100 - printInfo.percentFree << L"% used ";

	std::wcout << "| 'swap'=" << currentValue << BunitStr(printInfo.unit) << L";"
		<< printInfo.warn.pString(printInfo.tSwap) << L";" << printInfo.crit.pString(printInfo.tSwap)
		<< L";0;" << printInfo.tSwap << '\n';

	return state;
}

static int check_swap(printInfoStruct& printInfo)
{
	// Needs explicit cast: http://msinilo.pl/blog2/post/p1348/
	PENUM_PAGE_FILE_CALLBACKW pageFileCallback = (PENUM_PAGE_FILE_CALLBACKW)EnumPageFilesProc;
	std::vector<pageFileInfo> pageFiles;

	if(!EnumPageFilesW(pageFileCallback, &pageFiles)) {
		printErrorInfo();
		return 3;
	}

	for (int i = 0; i < pageFiles.size(); i++) {
		printInfo.tSwap += round(pageFiles.at(i).totalSwap / pow(1024.0, printInfo.unit));
		printInfo.aSwap += round(pageFiles.at(i).availableSpwap / pow(1024.0, printInfo.unit));
	}

	if (printInfo.aSwap > 0 && printInfo.tSwap > 0)
		printInfo.percentFree = 100.0 * printInfo.aSwap / printInfo.tSwap;
	else
		printInfo.percentFree = 0;

	return -1;
}

int wmain(int argc, WCHAR **argv)
{
	printInfoStruct printInfo = { };
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_swap(printInfo);
	if (ret != -1)
		return ret;

	return printOutput(printInfo);
}
