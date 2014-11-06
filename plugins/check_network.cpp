#include <Windows.h>
#include <Pdh.h>
#include <Shlwapi.h>
#include <iostream>
#include <pdhmsg.h>

#include "thresholds.h"

#include "boost\program_options.hpp"

#define VERSION 1.0
namespace po = boost::program_options;

using std::endl; using std::vector; using std::wstring;
using std::wcout; using std::cout;
struct nInterface {
	wstring name;
	long BytesInSec, BytesOutSec;
	nInterface(wstring p)
		: name(p)
	{}
};

struct printInfoStruct {
	threshold warn, crit;
};

static void die(const DWORD err=NULL);
static int parseArguments(int, TCHAR **, po::variables_map&, printInfoStruct&);
static int printOutput(printInfoStruct&, const vector<nInterface>&);
static int check_network(vector<nInterface>&);

int wmain(int argc, wchar_t **argv) {
	vector<nInterface> vInterfaces;
	printInfoStruct printInfo{ };
	po::variables_map vm;
	int ret = parseArguments(argc, argv, vm, printInfo);
	if (ret != -1)
		return ret;

	ret = check_network(vInterfaces);
	if (ret != -1)
		return ret;
		
	printOutput(printInfo, vInterfaces);
	return 1;
}

int parseArguments(int ac, wchar_t **av, po::variables_map& vm, printInfoStruct& printInfo) {
	wchar_t namePath[MAX_PATH];
	GetModuleFileName(NULL, namePath, MAX_PATH);
	wchar_t *progName = PathFindFileName(namePath);

	po::options_description desc("Options");

	desc.add_options()
		(",h", "print usage and exit")
		("help", "print help message and exit")
		("version,v", "print version and exit")
		("warning,w", po::wvalue<wstring>(), "warning value")
		("critical,c", po::wvalue<wstring>(), "critical value")
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
			L"%s is a simple program to check a machines network performance.\n"
			L"You can use the following options to define its behaviour:\n\n", progName);
		cout << desc;
		wprintf(
			L"\nIt will then output a string looking something like this:\n\n"
			L"\tNETWORK WARNING 1131B/s|network=1131B/s;1000;7000;0\n\n"
			L"\"DISK\" being the type of the check, \"WARNING\" the returned status\n"
			L"and \"1131B/s\" is the returned value.\n"
			L"The performance data is found behind the \"|\", in order:\n"
			L"returned value, warning threshold, critical threshold, minimal value and,\n"
			L"if applicable, the maximal value. Performance data will onl be displayed when\n"
			L"you set at least one threshold\n"
			L"This program will also print out additional performance data interface\n"
			L"by interface\n\n"
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

int printOutput(printInfoStruct& printInfo, const vector<nInterface>& vInterfaces) {
	long tIn = 0, tOut = 0;
	std::wstringstream tss;
	state state = OK;

	for (vector<nInterface>::const_iterator it = vInterfaces.begin(); it != vInterfaces.end(); ++it) {
		tIn += it->BytesInSec;
		tOut += it->BytesOutSec;
		tss << L"netI=\"" << it->name << L"\";in=" << it->BytesInSec << L"B/s;out=" << it->BytesOutSec << L"B/s ";
	}

	if (!printInfo.warn.set && !printInfo.crit.set) {
		wcout << L"NETWORK OK " << tIn+tOut << endl;
	}

	if (printInfo.warn.rend(tIn + tOut))
		state = WARNING;
	if (printInfo.crit.rend(tIn + tOut))
		state = CRITICAL;

	switch (state) {
	case OK:
		wcout << L"NETWORK OK " << tIn + tOut << L"B/s|" << tss.str() << endl;
		break;
	case WARNING:
		wcout << L"NETWORK WARNING " << tIn + tOut << L"B/s|" << tss.str() << endl;
		break;
	case CRITICAL:
		wcout << L"NETWORK CRITICAL " << tIn + tOut << L"B/s|" << tss.str() << endl;
		break;
	}

	return state;
}

int check_network(vector <nInterface>& vInterfaces) {
	const wchar_t *perfIn = L"\\Network Interface(*)\\Bytes Received/sec";
	const wchar_t *perfOut = L"\\Network Interface(*)\\Bytes Sent/sec";

	PDH_HQUERY phQuery = NULL;
	PDH_HCOUNTER phCounterIn, phCounterOut;
	DWORD dwBufferSizeIn = 0, dwBufferSizeOut = 0, dwItemCount = 0;
	PDH_FMT_COUNTERVALUE_ITEM *pDisplayValuesIn = NULL, *pDisplayValuesOut = NULL;

	if (PdhOpenQuery(NULL, NULL, &phQuery) != ERROR_SUCCESS)
		goto die;

	if (PdhOpenQuery(NULL, NULL, &phQuery) == ERROR_SUCCESS) {
		if (PdhAddEnglishCounter(phQuery, perfIn, NULL, &phCounterIn) == ERROR_SUCCESS) {
			if (PdhAddEnglishCounter(phQuery, perfOut, NULL, &phCounterOut) == ERROR_SUCCESS) {
				if (PdhCollectQueryData(phQuery) == ERROR_SUCCESS) {
					Sleep(1000);
					if (PdhCollectQueryData(phQuery) == ERROR_SUCCESS) {
						if (PdhGetFormattedCounterArray(phCounterIn, PDH_FMT_LONG, &dwBufferSizeIn, &dwItemCount, pDisplayValuesIn) == PDH_MORE_DATA &&
							PdhGetFormattedCounterArray(phCounterOut, PDH_FMT_LONG, &dwBufferSizeOut, &dwItemCount, pDisplayValuesOut) == PDH_MORE_DATA) {
							pDisplayValuesIn = new PDH_FMT_COUNTERVALUE_ITEM[dwItemCount*dwBufferSizeIn];
							pDisplayValuesOut = new  PDH_FMT_COUNTERVALUE_ITEM[dwItemCount*dwBufferSizeOut];
							if (PdhGetFormattedCounterArray(phCounterIn, PDH_FMT_LONG, &dwBufferSizeIn, &dwItemCount, pDisplayValuesIn) == ERROR_SUCCESS &&
								PdhGetFormattedCounterArray(phCounterOut, PDH_FMT_LONG, &dwBufferSizeOut, &dwItemCount, pDisplayValuesOut) == ERROR_SUCCESS) {
								for (DWORD i = 0; i < dwItemCount; i++) {
									nInterface *iface = new nInterface(wstring(pDisplayValuesIn[i].szName));
									iface->BytesInSec = pDisplayValuesIn[i].FmtValue.longValue;
									iface->BytesOutSec = pDisplayValuesOut[i].FmtValue.longValue;
									vInterfaces.push_back(*iface);
								}
								return -1;
							}
						}
					}
				}
			}
		}
	}

	
die:
	die();
	if (phQuery)
		PdhCloseQuery(phQuery);
	return 3;
}

void die(DWORD err) {
	if (!err)
		err = GetLastError();
	LPWSTR mBuf = NULL;
	size_t mS = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&mBuf, 0, NULL);
	wcout << mBuf << endl;
}