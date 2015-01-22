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

#include "thresholds.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <iostream>

using namespace boost::algorithm;

using std::wstring;

threshold::threshold(bool l)
	: set(false), legal(l)
{}

//return TRUE if the threshold is broken
bool threshold::rend(const double b)
{
	if (!set)
		return set;
	if (lower == upper)
		return b > upper == legal;
	else
		return (b < lower || upper < b) != legal;
}

//returns a printable string of the threshold
std::wstring threshold::pString()
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

threshold::threshold(const wstring& stri)
{
	if (stri.empty())
		throw std::invalid_argument("Threshold must not be empty");

	wstring str = stri;

	//kill whitespace
	boost::algorithm::trim(str);

	bool low = (str.at(0) == L'!');
	if (low)
		str = wstring(str.begin() + 1, str.end());

	bool pc = false;

	if (str.at(0) == L'[' && str.at(str.length() - 1) == L']') {//is range
		str = wstring(str.begin() + 1, str.end() - 1);
		std::vector<wstring> svec;
		boost::split(svec, str, boost::is_any_of(L"-"));
		if (svec.size() != 2)
			throw std::invalid_argument("Threshold range requires two arguments");
		wstring str1 = svec.at(0), str2 = svec.at(1);

		if (str1.at(str1.length() - 1) == L'%' && str2.at(str2.length() - 1) == L'%') {
			perc = true;
			str1 = wstring(str1.begin(), str1.end() - 1);
			str2 = wstring(str2.begin(), str2.end() - 1);
		}
			
		try {
			boost::algorithm::trim(str1);
			lower = boost::lexical_cast<double>(str1);
			boost::algorithm::trim(str2);
			upper = boost::lexical_cast<double>(str2);
			legal = !low; perc = pc; set = true;
		} catch (const boost::bad_lexical_cast&) {
			throw std::invalid_argument("Unknown Threshold type");
		}
	} else { //not range
		if (str.at(str.length() - 1) == L'%') {
			perc = true;
			str = wstring(str.begin(), str.end() - 1);
		}
		try {
			boost::algorithm::trim(str);
			lower = upper = boost::lexical_cast<double>(str);
			legal = !low; perc = pc; set = true;
		} catch (const boost::bad_lexical_cast&) {
			throw std::invalid_argument("Unknown Threshold type");
		}
	}
}

Bunit parseBUnit(const wstring& str)
{
	wstring wstr = to_upper_copy(str);

	if (wstr == L"B")
		return BunitB;
	if (wstr == L"kB")
		return BunitkB;
	if (wstr == L"MB")
		return BunitMB;
	if (wstr == L"GB")
		return BunitGB;
	if (wstr == L"TB")
		return BunitTB;

	throw std::invalid_argument("Unknown unit type");
}

wstring BunitStr(const Bunit& unit) 
{
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

Tunit parseTUnit(const wstring& str) {
	wstring wstr = to_lower_copy(str);

	if (wstr == L"ms")
		return TunitMS;
	if (wstr == L"s")
		return TunitS;
	if (wstr == L"m")
		return TunitM;
	if (wstr == L"h")
		return TunitH;

	throw std::invalid_argument("Unknown unit type");
}

wstring TunitStr(const Tunit& unit) 
{
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

void die(DWORD err)
{
	if (!err)
		err = GetLastError();
	LPWSTR mBuf = NULL;
	size_t mS = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
							  NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&mBuf, 0, NULL);
	std::wcout << mBuf << std::endl;
}