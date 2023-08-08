/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/debuginfo.hpp"

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
