// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DEBUGINFO_H
#define DEBUGINFO_H

#include "base/convert.hpp"
#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <fstream>

namespace icinga
{

/**
 * Debug information for a configuration element.
 *
 * @ingroup config
 */
struct DebugInfo
{
	String Path;

	int FirstLine{0};
	int FirstColumn{0};

	int LastLine{0};
	int LastColumn{0};
};

std::ostream& operator<<(std::ostream& out, const DebugInfo& val);

DebugInfo DebugInfoRange(const DebugInfo& start, const DebugInfo& end);

template<class T, unsigned long ExtraLines = 2>
void ShowCodeLocation(T& out, const DebugInfo& di, bool verbose = true)
{
	if (di.Path.IsEmpty())
		return;

	out << "Location: " << di;

	std::ifstream ifs;
	ifs.open(di.Path.CStr(), std::ifstream::in);

	int lineno = 0;
	char line[1024];

	while (ifs.good() && lineno <= di.LastLine + ExtraLines) {
		if (lineno == 0)
			out << "\n";

		lineno++;

		ifs.getline(line, sizeof(line));

		for (int i = 0; line[i]; i++)
			if (line[i] == '\t')
				line[i] = ' ';

		int extra_lines = verbose ? ExtraLines : 0;

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

}

#endif /* DEBUGINFO_H */
