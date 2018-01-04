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

#ifndef LIVESTATUSQUERY_H
#define LIVESTATUSQUERY_H

#include "livestatus/filter.hpp"
#include "livestatus/aggregator.hpp"
#include "base/object.hpp"
#include "base/array.hpp"
#include "base/stream.hpp"
#include "base/scriptframe.hpp"
#include <deque>

using namespace icinga;

namespace icinga
{

enum LivestatusError
{
	LivestatusErrorOK = 200,
	LivestatusErrorNotFound = 404,
	LivestatusErrorQuery = 452
};

/**
 * @ingroup livestatus
 */
class LivestatusQuery final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(LivestatusQuery);

	LivestatusQuery(const std::vector<String>& lines, const String& compat_log_path);

	bool Execute(const Stream::Ptr& stream);

	static int GetExternalCommands(void);

private:
	String m_Verb;

	bool m_KeepAlive;

	/* Parameters for GET queries. */
	String m_Table;
	std::vector<String> m_Columns;
	std::vector<String> m_Separators;

	Filter::Ptr m_Filter;
	std::deque<Aggregator::Ptr> m_Aggregators;

	String m_OutputFormat;
	bool m_ColumnHeaders;
	int m_Limit;

	String m_ResponseHeader;

	/* Parameters for COMMAND/SCRIPT queries. */
	String m_Command;
	String m_Session;

	/* Parameters for invalid queries. */
	int m_ErrorCode;
	String m_ErrorMessage;

	unsigned long m_LogTimeFrom;
	unsigned long m_LogTimeUntil;
	String m_CompatLogPath;

	void BeginResultSet(std::ostream& fp) const;
	void EndResultSet(std::ostream& fp) const;
	void AppendResultRow(std::ostream& fp, const Array::Ptr& row, bool& first_row) const;
	void PrintCsvArray(std::ostream& fp, const Array::Ptr& array, int level) const;
	void PrintPythonArray(std::ostream& fp, const Array::Ptr& array) const;
	static String QuoteStringPython(const String& str);

	void ExecuteGetHelper(const Stream::Ptr& stream);
	void ExecuteCommandHelper(const Stream::Ptr& stream);
	void ExecuteErrorHelper(const Stream::Ptr& stream);

	void SendResponse(const Stream::Ptr& stream, int code, const String& data);
	void PrintFixed16(const Stream::Ptr& stream, int code, const String& data);

	static Filter::Ptr ParseFilter(const String& params, unsigned long& from, unsigned long& until);
};

}

#endif /* LIVESTATUSQUERY_H */
