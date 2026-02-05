// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/debuginfo.hpp"
#include "base/convert.hpp"
#include <fstream>

using namespace icinga;

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
