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

#ifndef CHECK_PERFMON_H
#define CHECK_PERFMON_H

#include <Windows.h>
#include <Pdh.h>
#include <pdhmsg.h>

#include "thresholds.h"

#include "boost/program_options.hpp"

struct printInfoStruct
{
	threshold tWarn, tCrit;
	std::wstring wsFullPath;
	DOUBLE dValue;
	DWORD dwPerformanceWait = 1000,
		dwRequestedType = PDH_FMT_DOUBLE;
};

BOOL ParseArguments(CONST INT, WCHAR **, boost::program_options::variables_map&, printInfoStruct&);
BOOL GetIntstancesAndCountersOfObject(CONST std::wstring, std::vector<std::wstring>&, std::vector<std::wstring>&);
VOID PrintObjects();
VOID PrintObjectInfo(CONST printInfoStruct&);
INT QueryPerfData(printInfoStruct&);
INT PrintOutput(CONST boost::program_options::variables_map&, printInfoStruct&);
VOID FormatPDHError(PDH_STATUS);

#endif // !CHECK_PERFMON_H
