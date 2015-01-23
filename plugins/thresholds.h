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
#ifndef THRESHOLDS_H
#define THRESHOLDS_H
#include <string>
#include <Windows.h>
#include <vector>

enum Bunit { BunitB = 0, BunitkB = 1, BunitMB = 2, BunitGB = 3, BunitTB = 4 };
enum Tunit { TunitMS, TunitS, TunitM, TunitH };
enum state { OK = 0, WARNING = 1, CRITICAL = 2 };

class threshold
{
public:
	double lower, upper;
	//TRUE means everything BELOW upper/outside [lower-upper] is fine
	bool legal, perc, set;

	threshold();

	threshold(const double v, const double c, bool l = true, bool p = false);

	threshold(const std::wstring&);

	//return TRUE if the threshold is broken
	bool rend(const double val, const double max = 100);

	//returns a printable string of the threshold
	std::wstring pString(const double max = 100);

};
std::wstring removeZero(double);
std::vector<std::wstring> splitMultiOptions(std::wstring);

Bunit parseBUnit(const std::wstring&);
std::wstring BunitStr(const Bunit&);
Tunit parseTUnit(const std::wstring&);
std::wstring TunitStr(const Tunit&);

void die(DWORD err = 0);
#endif