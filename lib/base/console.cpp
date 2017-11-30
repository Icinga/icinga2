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

#include "base/console.hpp"
#include "base/initialize.hpp"
#include <iostream>

using namespace icinga;

static ConsoleType l_ConsoleType = Console_Dumb;

INITIALIZE_ONCE([]() {
	l_ConsoleType = Console_Dumb;

#ifndef _WIN32
	if (isatty(1))
		l_ConsoleType = Console_VT100;
#else /* _WIN32 */
	l_ConsoleType = Console_Windows;
#endif /* _WIN32 */
})

ConsoleColorTag::ConsoleColorTag(int color, ConsoleType consoleType)
	: m_Color(color), m_ConsoleType(consoleType)
{ }

std::ostream& icinga::operator<<(std::ostream& fp, const ConsoleColorTag& cct)
{
#ifndef _WIN32
	if (cct.m_ConsoleType == Console_VT100 || Console::GetType(fp) == Console_VT100)
		Console::PrintVT100ColorCode(fp, cct.m_Color);
#else /* _WIN32 */
	if (Console::GetType(fp) == Console_Windows) {
		fp.flush();
		Console::SetWindowsConsoleColor(fp, cct.m_Color);
	}
#endif /* _WIN32 */

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

#ifndef _WIN32
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
#else /* _WIN32 */
void Console::SetWindowsConsoleColor(std::ostream& fp, int color)
{
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	HANDLE hConsole;

	if (&fp == &std::cout)
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	else if (&fp == &std::cerr)
		hConsole = GetStdHandle(STD_ERROR_HANDLE);
	else
		return;

	if (!GetConsoleScreenBufferInfo(hConsole, &consoleInfo))
		return;

	WORD attrs = 0;

	if (color == Console_Normal)
		attrs = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

	switch (color & 0xff) {
		case Console_ForegroundBlack:
			attrs |= 0;
			break;
		case Console_ForegroundRed:
			attrs |= FOREGROUND_RED;
			break;
		case Console_ForegroundGreen:
			attrs |= FOREGROUND_GREEN;
			break;
		case Console_ForegroundYellow:
			attrs |= FOREGROUND_RED | FOREGROUND_GREEN;
			break;
		case Console_ForegroundBlue:
			attrs |= FOREGROUND_BLUE;
			break;
		case Console_ForegroundMagenta:
			attrs |= FOREGROUND_RED | FOREGROUND_BLUE;
			break;
		case Console_ForegroundCyan:
			attrs |= FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;
		case Console_ForegroundWhite:
			attrs |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;
	}

	switch (color & 0xff00) {
		case Console_BackgroundBlack:
			attrs |= 0;
			break;
		case Console_BackgroundRed:
			attrs |= BACKGROUND_RED;
			break;
		case Console_BackgroundGreen:
			attrs |= BACKGROUND_GREEN;
			break;
		case Console_BackgroundYellow:
			attrs |= BACKGROUND_RED | BACKGROUND_GREEN;
			break;
		case Console_BackgroundBlue:
			attrs |= BACKGROUND_BLUE;
			break;
		case Console_BackgroundMagenta:
			attrs |= BACKGROUND_RED | BACKGROUND_BLUE;
			break;
		case Console_BackgroundCyan:
			attrs |= BACKGROUND_GREEN | BACKGROUND_BLUE;
			break;
		case Console_BackgroundWhite:
			attrs |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
			break;
	}

	if (color & Console_Bold)
		attrs |= FOREGROUND_INTENSITY;

	SetConsoleTextAttribute(hConsole, attrs);
}
#endif /* _WIN32 */
