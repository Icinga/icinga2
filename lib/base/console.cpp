// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/console.hpp"
#include "base/initialize.hpp"
#include <iostream>

using namespace icinga;

static ConsoleType l_ConsoleType = Console_Dumb;

#ifdef _WIN32
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
#endif /* _WIN32 */

static void InitializeConsole()
{
	l_ConsoleType = Console_Dumb;

#ifndef _WIN32
	if (isatty(1))
		l_ConsoleType = Console_VT100;
#else /* _WIN32 */
	// Unlike isatty(1) on *nix, this isn't a passive check: VT100 processing is off by
	// default per-handle and has to be actively turned on for each of stdout/stderr.
	// l_ConsoleType is a single flag shared by both streams, so only use Console_VT100
	// if enabling it succeeds for both - otherwise whichever one failed would still
	// end up with raw escape codes written to it.
	bool vt100 = true;

	for (DWORD stdHandleId : {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE}) {
		HANDLE handle = GetStdHandle(stdHandleId);
		DWORD mode;

		if (!GetConsoleMode(handle, &mode) || !SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
			vt100 = false;
		}
	}

	if (vt100) {
		l_ConsoleType = Console_VT100;
	}
#endif /* _WIN32 */
}

INITIALIZE_ONCE(InitializeConsole);

ConsoleColorTag::ConsoleColorTag(int color, ConsoleType consoleType)
	: m_Color(color), m_ConsoleType(consoleType)
{ }

std::ostream& icinga::operator<<(std::ostream& fp, const ConsoleColorTag& cct)
{
	if (cct.m_ConsoleType == Console_VT100 || Console::GetType(fp) == Console_VT100)
		Console::PrintVT100ColorCode(fp, cct.m_Color);

	return fp;
}

void Console::SetType(std::ostream& fp, ConsoleType type)
{
	if (&fp == &std::cout || &fp == &std::cerr)
	l_ConsoleType = type;
}

ConsoleType Console::GetType(std::ostream& fp)
{
	if (&fp == &std::cout || &fp == &std::cerr)
		return l_ConsoleType;
	else
		return Console_Dumb;
}

void Console::PrintVT100ColorCode(std::ostream& fp, int color)
{
	if (color == Console_Normal) {
		fp << "\33[0m";
		return;
	}

	switch (color & 0xff) {
		case Console_ForegroundBlack:
			fp << "\33[30m";
			break;
		case Console_ForegroundRed:
			fp << "\33[31m";
			break;
		case Console_ForegroundGreen:
			fp << "\33[32m";
			break;
		case Console_ForegroundYellow:
			fp << "\33[33m";
			break;
		case Console_ForegroundBlue:
			fp << "\33[34m";
			break;
		case Console_ForegroundMagenta:
			fp << "\33[35m";
			break;
		case Console_ForegroundCyan:
			fp << "\33[36m";
			break;
		case Console_ForegroundWhite:
			fp << "\33[37m";
			break;
	}

	switch (color & 0xff00) {
		case Console_BackgroundBlack:
			fp << "\33[40m";
			break;
		case Console_BackgroundRed:
			fp << "\33[41m";
			break;
		case Console_BackgroundGreen:
			fp << "\33[42m";
			break;
		case Console_BackgroundYellow:
			fp << "\33[43m";
			break;
		case Console_BackgroundBlue:
			fp << "\33[44m";
			break;
		case Console_BackgroundMagenta:
			fp << "\33[45m";
			break;
		case Console_BackgroundCyan:
			fp << "\33[46m";
			break;
		case Console_BackgroundWhite:
			fp << "\33[47m";
			break;
	}

	if (color & Console_Bold)
		fp << "\33[1m";
}
