/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

enum Bunit { BunitB = 0, BunitkB = 1, BunitMB = 2, BunitGB = 3, BunitTB = 4 };
enum Tunit { TunitMS, TunitS, TunitM, TunitH };
enum state { OK = 0, WARNING = 1, CRITICAL = 2 };

class threshold
{
public:
	double lower, upper;
	//TRUE means everything BELOW upper/outside [lower-upper] is fine
	bool legal, perc, set;

	threshold(bool l = true)
		: set(false), legal(l) {}

	threshold(const double v, const double c, bool l = true, bool p = false)
		: lower(v), upper(c), legal(l), perc(p), set(true) {}

	//return TRUE if the threshold is broken
	bool rend(const double b)
	{
		if (!set)
			return set;
		if (lower == upper)
			return b > upper == legal;
		else
			return (b < lower || upper < b) != legal;
	}

	//returns a printable string of the threshold
	std::wstring pString()
	{
		if (!set)
			return L"0";

		std::wstring s;
		if (!legal)
			s.append(L"!");

		if (lower != upper) {
			if (perc)
				s.append(L"[").append(std::to_wstring(lower)).append(L"%").append(L"-")
				.append(std::to_wstring(upper)).append(L"%").append(L"]");
			else
				s.append(L"[").append(std::to_wstring(lower)).append(L"-")
				.append(std::to_wstring(upper)).append(L"]");
		} else {
			if (perc)
				s = std::to_wstring(lower).append(L"%");
			else
				s = std::to_wstring(lower);
		}
		return s;
	}
};

threshold parse(const std::wstring&);
Bunit parseBUnit(const wchar_t *);
std::wstring BunitStr(const Bunit&);
Tunit parseTUnit(const wchar_t *);
std::wstring TunitStr(const Tunit&);
#endif