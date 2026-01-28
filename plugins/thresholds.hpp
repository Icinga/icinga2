// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef THRESHOLDS_H
#define THRESHOLDS_H

#include <string>
#include <vector>
#include <windows.h>

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
	// doubles are always enough for ANY 64 bit value
	double lower;
	double upper;
	// true means everything BELOW upper/outside [lower-upper] is fine
	bool legal;
	bool perc;
	bool set;

	threshold();

	threshold(const double v, const double c, bool l = true, bool p = false);

	threshold(const std::wstring&);

	// returns true if the threshold is broken
	bool rend(const double val, const double max = 100.0);

	// returns a printable string of the threshold
	std::wstring pString(const double max = 100.0);

	threshold toSeconds(const Tunit& fromUnit);
};

std::wstring removeZero(double);
std::vector<std::wstring> splitMultiOptions(const std::wstring&);

Bunit parseBUnit(const std::wstring&);
std::wstring BunitStr(const Bunit&);
Tunit parseTUnit(const std::wstring&);
std::wstring TunitStr(const Tunit&);

void printErrorInfo(unsigned long err = 0);
std::wstring formatErrorInfo(unsigned long err);

std::wstring stateToString(const state&);

#endif /* THRESHOLDS_H */
