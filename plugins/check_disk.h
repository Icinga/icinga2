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

#ifndef CHECK_DISK_H
#define CHECK_DISK_H

#include <Windows.h>
#include <vector>

#include "boost/program_options.hpp"
#include "thresholds.h"

struct drive
{
	std::wstring name;
	double cap, free;
	drive(std::wstring p)
		: name(p)
	{
	}
};

struct printInfoStruct
{
	threshold warn, crit;
	std::vector<std::wstring> drives, exclude_drives;
	Bunit unit;
};

static INT parseArguments(int, wchar_t **, boost::program_options::variables_map&, printInfoStruct&);
static INT printOutput(printInfoStruct&, std::vector<drive>&);
static INT check_drives(std::vector<drive>&, std::vector<std::wstring>&);
static INT check_drives(std::vector<drive>&, printInfoStruct&);
static BOOL getFreeAndCap(drive&, const Bunit&);
static bool checkName(const drive& d, const std::wstring& s);
#endif /*CHECK_DISK_H*/
