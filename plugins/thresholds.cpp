// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "plugins/thresholds.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace boost::algorithm;

threshold::threshold()
	: set(false)
{}

threshold::threshold(const double v, const double c, bool l , bool p ) {
	lower = v;
	upper = c;
	legal = l;
	perc = p;
}

threshold::threshold(const std::wstring& stri)
{
	if (stri.empty())
		throw std::invalid_argument("Threshold must not be empty");

	std::wstring str = stri;

	//kill whitespace
	boost::algorithm::trim(str);

	bool low = (str.at(0) == L'!');
	if (low)
		str = std::wstring(str.begin() + 1, str.end());

	bool pc = false;

	if (str.at(0) == L'[' && str.at(str.length() - 1) == L']') {//is range
		str = std::wstring(str.begin() + 1, str.end() - 1);
		std::vector<std::wstring> svec;
		boost::split(svec, str, boost::is_any_of(L"-"));
		if (svec.size() != 2)
			throw std::invalid_argument("Threshold range requires two arguments");
		std::wstring str1 = svec.at(0), str2 = svec.at(1);

		if (str1.at(str1.length() - 1) == L'%' && str2.at(str2.length() - 1) == L'%') {
			pc = true;
			str1 = std::wstring(str1.begin(), str1.end() - 1);
			str2 = std::wstring(str2.begin(), str2.end() - 1);
		}

		try {
			boost::algorithm::trim(str1);
			lower = boost::lexical_cast<DOUBLE>(str1);
			boost::algorithm::trim(str2);
			upper = boost::lexical_cast<DOUBLE>(str2);
			legal = !low; perc = pc; set = true;
		} catch (const boost::bad_lexical_cast&) {
			throw std::invalid_argument("Unknown Threshold type");
		}
	} else { //not range
		if (str.at(str.length() - 1) == L'%') {
			pc = true;
			str = std::wstring(str.begin(), str.end() - 1);
		}
		try {
			boost::algorithm::trim(str);
			lower = upper = boost::lexical_cast<DOUBLE>(str);
			legal = !low; perc = pc; set = true;
		} catch (const boost::bad_lexical_cast&) {
			throw std::invalid_argument("Unknown Threshold type");
		}
	}
}

//return TRUE if the threshold is broken
bool threshold::rend(const double val, const double max)
{
	double upperAbs = upper;
	double lowerAbs = lower;

	if (perc) {
		upperAbs = upper / 100.0 * max;
		lowerAbs = lower / 100.0 * max;
	}

	if (!set)
		return set;
	if (lowerAbs == upperAbs)
		return val > upperAbs == legal;
	else
		return (val < lowerAbs || upperAbs < val) != legal;
}

//returns a printable string of the threshold
std::wstring threshold::pString(const double max)
{
	if (!set)
		return L"";
	//transform percentages to abolute values
	double lowerAbs = lower;
	double upperAbs = upper;
	if (perc) {
		lowerAbs = lower / 100.0 * max;
		upperAbs = upper / 100.0 * max;
	}

	std::wstring s, lowerStr = removeZero(lowerAbs),
					upperStr = removeZero(upperAbs);

	if (lower != upper) {
		s.append(L"[").append(lowerStr).append(L"-")
		.append(upperStr).append(L"]");
	} else
		s.append(lowerStr);

	return s;
}

threshold threshold::toSeconds(const Tunit& fromUnit) {
	if (!set)
		return *this;

	double lowerAbs = lower;
	double upperAbs = upper;

	switch (fromUnit) {
	case TunitMS:
		lowerAbs = lowerAbs / 1000;
		upperAbs = upperAbs / 1000;
		break;
	case TunitS:
		lowerAbs = lowerAbs ;
		upperAbs = upperAbs ;
		break;
	case TunitM:
		lowerAbs = lowerAbs * 60;
		upperAbs = upperAbs * 60;
		break;
	case TunitH:
		lowerAbs = lowerAbs * 60 * 60;
		upperAbs = upperAbs * 60 * 60;
		break;
	}

	return threshold(lowerAbs, upperAbs, legal, perc);
}

std::wstring removeZero(double val)
{
	std::wstring ret = boost::lexical_cast<std::wstring>(val);
	std::wstring::size_type pos = ret.length();
	if (ret.find_first_of(L".") == std::string::npos)
		return ret;
	for (std::wstring::reverse_iterator rit = ret.rbegin(); rit != ret.rend(); ++rit) {
		if (*rit == L'.') {
			return ret.substr(0, pos - 1);
		}
		if (*rit != L'0') {
			return ret.substr(0, pos);
		}
		pos--;
	}
	return L"0";
}

std::vector<std::wstring> splitMultiOptions(const std::wstring& str)
{
	std::vector<std::wstring> sVec;
	boost::split(sVec, str, boost::is_any_of(L","));
	return sVec;
}

Bunit parseBUnit(const std::wstring& str)
{
	std::wstring wstr = to_upper_copy(str);

	if (wstr == L"B")
		return BunitB;
	if (wstr == L"KB")
		return BunitkB;
	if (wstr == L"MB")
		return BunitMB;
	if (wstr == L"GB")
		return BunitGB;
	if (wstr == L"TB")
		return BunitTB;

	throw std::invalid_argument("Unknown unit type");
}

std::wstring BunitStr(const Bunit& unit)
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

Tunit parseTUnit(const std::wstring& str) {
	std::wstring wstr = to_lower_copy(str);

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

std::wstring TunitStr(const Tunit& unit)
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

void printErrorInfo(unsigned long err)
{
	if (!err)
		err = GetLastError();
	LPWSTR mBuf = NULL;
	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&mBuf, 0, NULL))
		std::wcout << "Failed to format error message, last error was: " << err << '\n';
	else {
		boost::trim_right(std::wstring(mBuf));
		std::wcout << mBuf << std::endl;
	}
}

std::wstring formatErrorInfo(unsigned long err) {
	std::wostringstream out;
	if (!err)
		err = GetLastError();
	LPWSTR mBuf = NULL;
	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&mBuf, 0, NULL))
		out << "Failed to format error message, last error was: " << err;
	else {
		std::wstring tempOut = std::wstring(mBuf);
		boost::trim_right(tempOut);
		out << tempOut;
	}

	return out.str();
}

std::wstring stateToString(const state& state) {
	switch (state) {
		case OK: return L"OK";
		case WARNING: return L"WARNING";
		case CRITICAL: return L"CRITICAL";
		default: return L"UNKNOWN";
	}
}