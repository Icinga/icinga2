#include <Pdh.h>
#include <Shlwapi.h>
#include <pdhmsg.h>
#include <iostream>

#include "thresholds.h"

#include "boost\program_options.hpp"

#define VERSION 1.0

namespace po = boost::program_options;

using std::endl; using std::cout; using std::wstring;
using std::wcout;

struct printInfoStruct {
	threshold warn, crit;
	double load;
};

static int parseArguments(int, wchar_t **, po::variables_map&, printInfoStruct&);
static int printOutput(printInfoStruct&);
static int check_load(printInfoStruct&);

int wmain(int argc, wchar_t **argv) {
	printInfoStruct printInfo{ };
	po::variables_map vm;

	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_load(printInfo);
	if (ret != -1)
		return ret;

	printOutput(printInfo);
	return 1;
}

int parseArguments(int ac, wchar_t **av, po::variables_map& vm, printInfoStruct& printInfo) {
	wchar_t namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	wchar_t *progName = PathFindFileName(namePath);

	po::options_description desc;

	desc.add_options()
		(",h", "print usage message and exit")
		("help", "print help message and exit")
		("version,v", "print version and exit")
		("warning,w", po::wvalue<wstring>(), "warning value (in percent)")
		("critical,c", po::wvalue<wstring>(), "critical value (in percent)")
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
	}

	catch (std::exception& e) {
		cout << e.what() << endl << desc << endl;
		return 3;
	}

	if (vm.count("h")) {
		cout << desc << endl;
		return 0;
	}

	if (vm.count("help")) {
		wcout << progName << " Help\n\tVersion: " << VERSION << endl;
		wprintf(
			L"%s is a simple program to check a machines CPU load.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tLOAD WARNING 67%%|load=67%%;50%%;90%%;0;100\n\n"
			L"\"LOAD\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"67%%\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. Performance data will onl be displayed when\n"
			L"you set at least one threshold\n"
			L"%s' exit codes denote the following:\n"
			L" 0\tOK,\n\tno Thresholds were broken or the programs check part was not executed\n"
			L" 1\tWARNING,\n\tThe warning, but not the critical threshold was broken\n"
			L" 2\tCRITICAL,\n\tThe critical threshold was broken\n"
			L" 3\tUNKNOWN, \n\tThe programme experienced an internal or input error\n\n"
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

	if (vm.count("version"))
		cout << "Version: " << VERSION << endl;

	if (vm.count("warning")) 
		printInfo.warn = parse(vm["warning"].as<wstring>());

	if (vm.count("critical"))
		printInfo.crit = parse(vm["critical"].as<wstring>());

	return -1;
}

int printOutput(printInfoStruct& printInfo) {
	state state = OK;
	
	if (!printInfo.warn.set && !printInfo.crit.set) {
		wcout << L"LOAD OK " << printInfo.load << endl;
	}

	if (printInfo.warn.rend(printInfo.load))
		state = WARNING;

	if (printInfo.crit.rend(printInfo.load))
		state = CRITICAL;

	std::wstringstream perf;
	perf << L"%|load=" << printInfo.load << L"%;" << printInfo.warn.pString() << L";" 
		<< printInfo.crit.pString() << L";0;100" << endl;

	switch (state) {
	case OK:
		wcout << L"LOAD OK " << printInfo.load << perf.str();
		break;
	case WARNING:
		wcout << L"LOAD WARNING " << printInfo.load << perf.str();
		break;
	case CRITICAL:
		wcout << L"LOAD CRITICAL " << printInfo.load << perf.str();
		break;
	}

	return state;
}

int check_load(printInfoStruct& printInfo) {
	PDH_HQUERY phQuery;
	PDH_HCOUNTER phCounter;
	DWORD dwBufferSize = 0;
	DWORD CounterType;
	PDH_FMT_COUNTERVALUE DisplayValue;

	LPCWSTR path = L"\\Processor(_Total)\\% Idle Time";

	if (PdhOpenQuery(NULL, NULL, &phQuery) != ERROR_SUCCESS)
		goto cleanup;

	if (PdhAddEnglishCounter(phQuery, path, NULL, &phCounter) != ERROR_SUCCESS)
		goto cleanup;

	if (PdhCollectQueryData(phQuery) != ERROR_SUCCESS)
		goto cleanup;

	Sleep(1000);

	if (PdhCollectQueryData(phQuery) != ERROR_SUCCESS)
		goto cleanup;

	if (PdhGetFormattedCounterValue(phCounter, PDH_FMT_DOUBLE, &CounterType, &DisplayValue) == ERROR_SUCCESS) {
		if (DisplayValue.CStatus == PDH_CSTATUS_VALID_DATA)
			printInfo.load = 100.0 - DisplayValue.doubleValue;
		PdhCloseQuery(phQuery);
		return -1;
	}

cleanup:
	if (phQuery)
		PdhCloseQuery(phQuery);
	return 3;
}