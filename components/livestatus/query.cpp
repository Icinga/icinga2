/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-livestatus.h"

using namespace icinga;
using namespace livestatus;

Query::Query(const vector<String>& lines)
	: m_KeepAlive(false), m_OutputFormat("csv"), m_ColumnHeaders(true), m_Limit(-1)
{
	String line = lines[0];

	size_t sp_index = line.FindFirstOf(" ");

	if (sp_index == String::NPos)
		BOOST_THROW_EXCEPTION(runtime_error("Livestatus header must contain a verb."));

	String verb = line.SubStr(0, sp_index);
	String target = line.SubStr(sp_index + 1);

	m_Verb = verb;

	if (m_Verb == "COMMAND") {
		m_Command = target;
	} else if (m_Verb == "GET") {
		m_Table = target;
	} else {
		m_Verb = "ERROR";
		m_ErrorCode = 452;
		m_ErrorMessage = "Unknown livestatus verb: " + m_Verb;
		return;
	}

	deque<Filter::Ptr> filters, stats;

	for (unsigned int i = 1; i < lines.size(); i++) {
		line = lines[i];

		size_t col_index = line.FindFirstOf(":");
		String header = line.SubStr(0, col_index);
		String params = line.SubStr(col_index + 2);

		if (header == "ResponseHeader")
			m_ResponseHeader = params;
		else if (header == "OutputFormat")
			m_OutputFormat = params;
		else if (header == "Columns")
			m_Columns = params.Split(is_any_of(" "));
		else if (header == "ColumnHeaders")
			m_ColumnHeaders = (params == "on");
		else if (header == "Filter" || header == "Stats") {
			vector<String> tokens = params.Split(is_any_of(" "));

			if (tokens.size() < 3) {
				m_Verb = "ERROR";
				m_ErrorCode = 452;
				m_ErrorMessage = "Expected 3 parameters in the filter specification.";
				return;
			}

			String op = tokens[1];
			bool negate = false;

			if (op == "!=") {
				op = "=";
				negate = true;
			} else if (op == "!~") {
				op = "~";
				negate = true;
			} else if (op == "!=~") {
				op = "=~";
				negate = true;
			} else if (op == "!~~") {
				op = "~~";
				negate = true;
			}

			Filter::Ptr filter = boost::make_shared<AttributeFilter>(tokens[0], op, tokens[2]);

			if (negate)
				filter = boost::make_shared<NegateFilter>(filter);

			deque<Filter::Ptr>& deq = (header == "Filter") ? filters : stats;
			deq.push_back(filter);
		} else if (header == "Or" || header == "And") {
			deque<Filter::Ptr>& deq = (header == "Or" || header == "And") ? filters : stats;

			int num = Convert::ToLong(params);
			CombinerFilter::Ptr filter;

			if (header == "Or" || header == "StatsOr")
				filter = boost::make_shared<OrFilter>();
			else
				filter = boost::make_shared<AndFilter>();

			if (num > deq.size()) {
				m_Verb = "ERROR";
				m_ErrorCode = 451;
				m_ErrorMessage = "Or/StatsOr is referencing " + Convert::ToString(num) + " filters; stack only contains " + Convert::ToString(deq.size()) + " filters";
				return;
			}

			while (num--) {
				filter->AddSubFilter(deq.back());
				deq.pop_back();
			}

			deq.push_back(filter);
		} else if (header == "Negate" || header == "StatsNegate") {
			deque<Filter::Ptr>& deq = (header == "Negate") ? filters : stats;

			if (deq.empty()) {
				m_Verb = "ERROR";
				m_ErrorCode = 451;
				m_ErrorMessage = "Negate/StatsNegate used, however the filter stack is empty";
				return;
			}

			Filter::Ptr filter = deq.back();
			filters.pop_back();

			deq.push_back(boost::make_shared<NegateFilter>(filter));
		}
	}

	/* Combine all top-level filters into a single filter. */
	AndFilter::Ptr top_filter = boost::make_shared<AndFilter>();

	BOOST_FOREACH(const Filter::Ptr& filter, filters) {
		top_filter->AddSubFilter(filter);
	}

	m_Filter = top_filter;
	m_Stats.swap(stats);
}

void Query::PrintResultSet(ostream& fp, const vector<String>& columns, const Array::Ptr& rs)
{
	if (m_OutputFormat == "csv" && m_Columns.size() == 0 && m_ColumnHeaders) {
		bool first = true;

		BOOST_FOREACH(const String& column, columns) {
			if (first)
				first = false;
			else
				fp << ";";

			fp << column;
		}

		fp << "\n";
	}

	if (m_OutputFormat == "csv") {
		ObjectLock olock(rs);

		BOOST_FOREACH(const Array::Ptr& row, rs) {
			bool first = true;

			ObjectLock rlock(row);
			BOOST_FOREACH(const Value& value, row) {
				if (first)
					first = false;
				else
					fp << ";";

				fp << Convert::ToString(value);
			}

			fp << "\n";
		}
	} else if (m_OutputFormat == "json") {
		fp << Value(rs).Serialize();
	}
}

void Query::ExecuteGetHelper(const Stream::Ptr& stream)
{
	Logger::Write(LogInformation, "livestatus", "Table: " + m_Table);

	Table::Ptr table = Table::GetByName(m_Table);

	if (!table) {
		SendResponse(stream, 404, "Table '" + m_Table + "' does not exist.");

		return;
	}

	vector<Object::Ptr> objects = table->FilterRows(m_Filter);
	vector<String> columns;
	
	if (m_Columns.size() > 0)
		columns = m_Columns;
	else
		columns = table->GetColumnNames();

	Array::Ptr rs = boost::make_shared<Array>();

	if (m_Stats.empty()) {
		BOOST_FOREACH(const Object::Ptr& object, objects) {
			Array::Ptr row = boost::make_shared<Array>();

			BOOST_FOREACH(const String& column, columns) {
				Table::ColumnAccessor accessor = table->GetColumn(column);

				if (accessor.empty()) {
					SendResponse(stream, 450, "Column '" + column + "' does not exist.");

					return;
				}

				row->Add(accessor(object));
			}

			rs->Add(row);
		}
	} else {
		vector<int> stats(m_Stats.size(), 0);

		BOOST_FOREACH(const Object::Ptr& object, objects) {
			int index = 0;
			BOOST_FOREACH(const Filter::Ptr filter, m_Stats) {
				if (filter->Apply(table, object))
					stats[index]++;

				index++;
			}
		}

		Array::Ptr row = boost::make_shared<Array>();
		for (int i = 0; i < m_Stats.size(); i++)
			row->Add(stats[i]);

		rs->Add(row);

		m_ColumnHeaders = false;
	}

	stringstream result;
	PrintResultSet(result, columns, rs);

	SendResponse(stream, 200, result.str());
}

void Query::ExecuteCommandHelper(const Stream::Ptr& stream)
{
	Logger::Write(LogInformation, "livestatus", "Executing command: " + m_Command);
	ExternalCommandProcessor::Execute(m_Command);
	SendResponse(stream, 200, "");
}

void Query::ExecuteErrorHelper(const Stream::Ptr& stream)
{
	SendResponse(stream, m_ErrorCode, m_ErrorMessage);
}

void Query::SendResponse(const Stream::Ptr& stream, int code, const String& data)
{
	if (m_ResponseHeader == "fixed16")
		PrintFixed16(stream, code, data);

	if (m_ResponseHeader == "fixed16" || code == 200)
		stream->Write(data.CStr(), data.GetLength());
}

void Query::PrintFixed16(const Stream::Ptr& stream, int code, const String& data)
{
	ASSERT(code >= 100 && code <= 999);

	String sCode = Convert::ToString(code);
	String sLength = Convert::ToString(data.GetLength());

	String header = sCode + String(16 - 3 - sLength.GetLength() - 1, ' ') + sLength + "\n";
	stream->Write(header.CStr(), header.GetLength());
}

void Query::Execute(const Stream::Ptr& stream)
{
	try {
	Logger::Write(LogInformation, "livestatus", "Executing livestatus query: " + m_Verb);

	if (m_Verb == "GET")
		ExecuteGetHelper(stream);
	else if (m_Verb == "COMMAND")
		ExecuteCommandHelper(stream);
	else if (m_Verb == "ERROR")
		ExecuteErrorHelper(stream);
	else
		BOOST_THROW_EXCEPTION(runtime_error("Invalid livestatus query verb."));
	} catch (const std::exception& ex) {
		SendResponse(stream, 452, boost::diagnostic_information(ex));
	}

	if (!m_KeepAlive)
		stream->Close();
}
