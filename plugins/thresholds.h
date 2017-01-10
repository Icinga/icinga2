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

#ifndef THRESHOLDS_H
#define THRESHOLDS_H

#include <string>
#include <Windows.h>
#include <vector>

enum Bunit
{
	BunitB = 0, BunitkB = 1, BunitMB = 2, BunitGB = 3, BunitTB = 4
};

enum Tunit
{
	TunitMS, TunitS, TunitM, TunitH
};

enum state
{
	OK = 0, WARNING = 1, CRITICAL = 2
};

class threshold
{
public:
	//DOUBLEs are always enough for ANY 64 bit value
	DOUBLE lower, upper;
	//TRUE means everything BELOW upper/outside [lower-upper] is fine
	BOOL legal, perc, set;

	threshold();

	threshold(CONST DOUBLE v, CONST DOUBLE c, BOOL l = TRUE, BOOL p = FALSE);

	threshold(CONST std::wstring&);

	//return TRUE if the threshold is broken
	BOOL rend(CONST DOUBLE val, CONST DOUBLE max = 100.0);

	//returns a printable string of the threshold
	std::wstring pString(CONST DOUBLE max = 100.0);

};
std::wstring removeZero(DOUBLE);
std::vector<std::wstring> splitMultiOptions(std::wstring);

Bunit parseBUnit(CONST std::wstring&);
std::wstring BunitStr(CONST Bunit&);
Tunit parseTUnit(CONST std::wstring&);
std::wstring TunitStr(CONST Tunit&);

VOID die(DWORD err = 0);

#endif /* THRESHOLDS_H */
