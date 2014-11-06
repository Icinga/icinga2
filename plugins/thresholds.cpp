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
#include <vector>

#include "thresholds.h"

#include "boost\algorithm\string.hpp"
#include "boost\lexical_cast.hpp"

using std::wstring;

threshold parse(const wstring& stri)
{
	if (stri.empty())
		throw std::invalid_argument("thresholds must not be empty");

	wstring str = stri;

	bool low = (str.at(0) == L'!');
	if (low)
		str = wstring(str.begin() + 1, str.end());

	bool perc = false;

	if (str.at(0) == L'[' && str.at(str.length() - 1) == L']') {//is range
		str = wstring(str.begin() + 1, str.end() - 1);
		std::vector<wstring> svec;
		boost::split(svec, str, boost::is_any_of(L"-"));
		if (svec.size() != 2)
			throw std::invalid_argument("threshold range requires two arguments");
		wstring str1 = svec.at(0), str2 = svec.at(1);

		if (str1.at(str1.length() - 1) == L'%' && str2.at(str2.length() - 1) == L'%') {
			perc = true;
			str1 = wstring(str1.begin(), str1.end() - 1);
			str2 = wstring(str2.begin(), str2.end() - 1);
		}
			
		try {
			double d1 = boost::lexical_cast<double>(str1);
			double d2 = boost::lexical_cast<double>(str2);
			return threshold(d1, d2, !low, perc);
		} catch (const boost::bad_lexical_cast&) {
			throw std::invalid_argument("threshold must be a number");
		}
	} else { //not range
		if (str.at(str.length() - 1) == L'%') {
			perc = true;
			str = wstring(str.begin(), str.end() - 1);
		}
		try {
			double d = boost::lexical_cast<double>(str);
			return threshold(d, d, !low, perc);

		} catch (const boost::bad_lexical_cast&) {
			throw std::invalid_argument("threshold must be a number");
		}
	}
}

Bunit parseBUnit(const wchar_t *str)
{
	if (!wcscmp(str, L"B"))
		return BunitB;
	if (!wcscmp(str, L"kB"))
		return BunitkB;
	if (!wcscmp(str, L"MB"))
		return BunitMB;
	if (!wcscmp(str, L"GB"))
		return BunitGB;
	if (!wcscmp(str, L"TB"))
		return BunitTB;

	throw std::invalid_argument("Unknown unit type");
}

wstring BunitStr(const Bunit& unit) {
	switch (unit) {
	case BunitB:
		return L"B";
	case BunitkB:
		return L"kB";
	case BunitMB:
		return L"MB";
	case BunitGB:
		return L"GB";
	case BunitTB:
		return L"TB";
	}
	return NULL;
}

Tunit parseTUnit(const wchar_t *str) {
	if (!wcscmp(str, L"ms"))
		return TunitMS;
	if (!wcscmp(str, L"s"))
		return TunitS;
	if (!wcscmp(str, L"m"))
		return TunitM;
	if (!wcscmp(str, L"h"))
		return TunitH;

	throw std::invalid_argument("Unknown unit type");
}

wstring TunitStr(const Tunit& unit) {
	switch (unit) {
	case TunitMS:
		return L"ms";
	case TunitS:
		return L"s";
	case TunitM:
		return L"m";
	case TunitH:
		return L"h";
	}
	return NULL;
}