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

#ifndef CHECK_PING_H
#define CHECK_PING_H
#include "thresholds.h"
#include "boost/program_options.hpp"  

struct response
{
	DOUBLE avg;
	UINT pMin = 0, pMax = 0, dropped = 0;
};

struct printInfoStruct
{
	threshold warn, crit;
	threshold wpl, cpl;
	std::wstring host, ip;
	BOOL ipv6 = FALSE;
	DWORD timeout = 1000;
	INT num = 5;
};

INT printOutput(printInfoStruct&, response&);
INT parseArguments(INT, WCHAR **, boost::program_options::variables_map&, printInfoStruct&);
INT check_ping4(CONST printInfoStruct&, response&);
INT check_ping6(CONST printInfoStruct&, response&);
BOOL resolveHostname(CONST std::wstring, BOOL, std::wstring&);

#endif // !CHECK_PING_H
