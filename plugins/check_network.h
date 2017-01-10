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
 
#ifndef CHECK_NETWORK_H
#define CHECK_NETWORK_H
#include <vector>

#include "thresholds.h"
#include "boost/program_options.hpp"

struct nInterface
{
	std::wstring name;
	LONG BytesInSec, BytesOutSec;
	nInterface(std::wstring p)
		: name(p)
	{
	}
};

struct printInfoStruct
{
	threshold warn, crit;
};

INT parseArguments(INT, WCHAR **, boost::program_options::variables_map&, printInfoStruct&);
INT printOutput(printInfoStruct&, CONST std::vector<nInterface>&, CONST std::map<std::wstring, std::wstring>&);
INT check_network(std::vector<nInterface>&);
BOOL mapSystemNamesToFamiliarNames(std::map<std::wstring, std::wstring>&);

#endif // !CHECK_NETWORK_H
