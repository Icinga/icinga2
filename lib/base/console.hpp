// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONSOLE_H
#define CONSOLE_H

#include "base/i2-base.hpp"
#include <iosfwd>

namespace icinga
{

enum ConsoleColor
{
	Console_Normal,

	// bit 0-7: foreground
	Console_ForegroundBlack = 1,
	Console_ForegroundRed = 2,
	Console_ForegroundGreen = 3,
	Console_ForegroundYellow = 4,
	Console_ForegroundBlue = 5,
	Console_ForegroundMagenta = 6,
	Console_ForegroundCyan = 7,
	Console_ForegroundWhite = 8,

	// bit 8-15: background
	Console_BackgroundBlack = 256,
	Console_BackgroundRed = 266,
	Console_BackgroundGreen = 267,
	Console_BackgroundYellow = 268,
	Console_BackgroundBlue = 269,
	Console_BackgroundMagenta = 270,
	Console_BackgroundCyan = 271,
	Console_BackgroundWhite = 272,

	// bit 16-23: flags
	Console_Bold = 65536
};

enum ConsoleType
{
	Console_Autodetect = -1,

	Console_Dumb,
#ifndef _WIN32
	Console_VT100,
#else /* _WIN32 */
	Console_Windows,
#endif /* _WIN32 */
};

class ConsoleColorTag
{
public:
	ConsoleColorTag(int color, ConsoleType consoleType = Console_Autodetect);

	friend std::ostream& operator<<(std::ostream& fp, const ConsoleColorTag& cct);

private:
	int m_Color;
	int m_ConsoleType;
};

std::ostream& operator<<(std::ostream& fp, const ConsoleColorTag& cct);

/**
 * Console utilities.
 *
 * @ingroup base
 */
class Console
{
public:
	static void DetectType();

	static void SetType(std::ostream& fp, ConsoleType type);
	static ConsoleType GetType(std::ostream& fp);

#ifndef _WIN32
	static void PrintVT100ColorCode(std::ostream& fp, int color);
#else /* _WIN32 */
	static void SetWindowsConsoleColor(std::ostream& fp, int color);
#endif /* _WIN32 */

private:
	Console();
};

}

#endif /* CONSOLE_H */
