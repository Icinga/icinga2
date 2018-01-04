/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/debuginfo.hpp"
#include "base/convert.hpp"
#include <fstream>

using namespace icinga;

DebugInfo::DebugInfo()
	: FirstLine(0), FirstColumn(0), LastLine(0), LastColumn(0)
{ }

/**
 * Outputs a DebugInfo struct to a stream.
 *
 * @param out The output stream.
 * @param val The DebugInfo struct.
 * @returns The output stream.
 */
std::ostream& icinga::operator<<(std::ostream& out, const DebugInfo& val)
{
	out << "in " << val.Path << ": "
		<< val.FirstLine << ":" << val.FirstColumn
		<< "-"
		<< val.LastLine << ":" << val.LastColumn;

	return out;
}

DebugInfo icinga::DebugInfoRange(const DebugInfo& start, const DebugInfo& end)
{
	DebugInfo result;
	result.Path = start.Path;
	result.FirstLine = start.FirstLine;
	result.FirstColumn = start.FirstColumn;
	result.LastLine = end.LastLine;
	result.LastColumn = end.LastColumn;
	return result;
}

#define EXTRA_LINES 2

void icinga::ShowCodeLocation(std::ostream& out, const DebugInfo& di, bool verbose)
{
	if (di.Path.IsEmpty())
		return;

	out << "Location: " << di;

	std::ifstream ifs;
	ifs.open(di.Path.CStr(), std::ifstream::in);

	int lineno = 0;
	char line[1024];

	while (ifs.good() && lineno <= di.LastLine + EXTRA_LINES) {
		if (lineno == 0)
			out << "\n";

		lineno++;

		ifs.getline(line, sizeof(line));

		for (int i = 0; line[i]; i++)
			if (line[i] == '\t')
				line[i] = ' ';

		int extra_lines = verbose ? EXTRA_LINES : 0;

		if (lineno < di.FirstLine - extra_lines || lineno > di.LastLine + extra_lines)
			continue;

		String pathInfo = di.Path + "(" + Convert::ToString(lineno) + "): ";
		out << pathInfo;
		out << line << "\n";

		if (lineno >= di.FirstLine && lineno <= di.LastLine) {
			int start, end;

			start = 0;
			end = strlen(line);

			if (lineno == di.FirstLine)
				start = di.FirstColumn - 1;

			if (lineno == di.LastLine)
				end = di.LastColumn;

			if (start < 0) {
				end -= start;
				start = 0;
			}

			out << String(pathInfo.GetLength(), ' ');
			out << String(start, ' ');
			out << String(end - start, '^');

			out << "\n";
		}
	}
}

