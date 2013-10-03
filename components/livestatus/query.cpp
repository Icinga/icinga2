/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "livestatus/query.h"
#include "livestatus/countaggregator.h"
#include "livestatus/sumaggregator.h"
#include "livestatus/minaggregator.h"
#include "livestatus/maxaggregator.h"
#include "livestatus/avgaggregator.h"
#include "livestatus/stdaggregator.h"
#include "livestatus/invsumaggregator.h"
#include "livestatus/invavgaggregator.h"
#include "livestatus/attributefilter.h"
#include "livestatus/negatefilter.h"
#include "livestatus/orfilter.h"
#include "livestatus/andfilter.h"
#include "icinga/externalcommandprocessor.h"
#include "base/debug.h"
#include "base/convert.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace icinga;
using namespace livestatus;

static int l_ExternalCommands = 0;
static boost::mutex l_QueryMutex;

Query::Query(const std::vector<String>& lines)
	: m_KeepAlive(false), m_OutputFormat("csv"), m_ColumnHeaders(true), m_Limit(-1)
{
	if (lines.size() == 0) {
		m_Verb = "ERROR";
		m_ErrorCode = LivestatusErrorQuery;
		m_ErrorMessage = "Empty Query. Aborting.";
		return;
	}

	/* default separators */
	m_Separators.push_back("\n");
	m_Separators.push_back(";");
	m_Separators.push_back(",");
	m_Separators.push_back("|");

	String line = lines[0];

	size_t sp_index = line.FindFirstOf(" ");

	if (sp_index == String::NPos)
		BOOST_THROW_EXCEPTION(std::runtime_error("Livestatus header must contain a verb."));

	String verb = line.SubStr(0, sp_index);
	String target = line.SubStr(sp_index + 1);

	m_Verb = verb;

	if (m_Verb == "COMMAND") {
		m_Command = target;
	} else if (m_Verb == "GET") {
		m_Table = target;
	} else {
		m_Verb = "ERROR";
		m_ErrorCode = LivestatusErrorQuery;
		m_ErrorMessage = "Unknown livestatus verb: " + m_Verb;
		return;
	}

	std::deque<Filter::Ptr> filters, stats;
	std::deque<Aggregator::Ptr> aggregators;

	for (unsigned int i = 1; i < lines.size(); i++) {
		line = lines[i];

		size_t col_index = line.FindFirstOf(":");
		String header = line.SubStr(0, col_index);
		String params;
		std::vector<String> separators;

		if (line.GetLength() > col_index + 2)
			params = line.SubStr(col_index + 2);

		if (header == "ResponseHeader")
			m_ResponseHeader = params;
		else if (header == "OutputFormat")
			m_OutputFormat = params;
		else if (header == "Columns")
			boost::algorithm::split(m_Columns, params, boost::is_any_of(" "));
		else if (header == "Separators")
			boost::algorithm::split(separators, params, boost::is_any_of(" "));
			/* ugly ascii long to char conversion, but works */
			if (separators.size() > 0)
				m_Separators[0] = String(1, static_cast<char>(Convert::ToLong(separators[0])));
			if (separators.size() > 1)
				m_Separators[1] = String(1, static_cast<char>(Convert::ToLong(separators[1])));
			if (separators.size() > 2)
				m_Separators[2] = String(1, static_cast<char>(Convert::ToLong(separators[2])));
			if (separators.size() > 3)
				m_Separators[3] = String(1, static_cast<char>(Convert::ToLong(separators[3])));
		else if (header == "ColumnHeaders")
			m_ColumnHeaders = (params == "on");
		else if (header == "Filter") {
			Filter::Ptr filter = ParseFilter(params);

			if (!filter) {
				m_Verb = "ERROR";
				m_ErrorCode = LivestatusErrorQuery;
				m_ErrorMessage = "Invalid filter specification: " + line;
				return;
			}

			filters.push_back(filter);
		} else if (header == "Stats") {
			std::vector<String> tokens;
			boost::algorithm::split(tokens, params, boost::is_any_of(" "));

			if (tokens.size() < 2) {
				m_Verb = "ERROR";
				m_ErrorCode = LivestatusErrorQuery;
				m_ErrorMessage = "Missing aggregator column name: " + line;
				return;
			}

			String aggregate_arg = tokens[0];
			String aggregate_attr = tokens[1];

			Aggregator::Ptr aggregator;
			Filter::Ptr filter;

			if (aggregate_arg == "sum") {
				aggregator = boost::make_shared<SumAggregator>(aggregate_attr);
			} else if (aggregate_arg == "min") {
				aggregator = boost::make_shared<MinAggregator>(aggregate_attr);
			} else if (aggregate_arg == "max") {
				aggregator = boost::make_shared<MaxAggregator>(aggregate_attr);
			} else if (aggregate_arg == "avg") {
				aggregator = boost::make_shared<AvgAggregator>(aggregate_attr);
			} else if (aggregate_arg == "std") {
				aggregator = boost::make_shared<StdAggregator>(aggregate_attr);
			} else if (aggregate_arg == "suminv") {
				aggregator = boost::make_shared<InvSumAggregator>(aggregate_attr);
			} else if (aggregate_arg == "avginv") {
				aggregator = boost::make_shared<InvAvgAggregator>(aggregate_attr);
			} else {
				filter = ParseFilter(params);

				if (!filter) {
					m_Verb = "ERROR";
					m_ErrorCode = LivestatusErrorQuery;
					m_ErrorMessage = "Invalid filter specification: " + line;
					return;
				}

				aggregator = boost::make_shared<CountAggregator>();
			}

			aggregator->SetFilter(filter);
			aggregators.push_back(aggregator);

			stats.push_back(filter);
		} else if (header == "Or" || header == "And") {
			std::deque<Filter::Ptr>& deq = (header == "Or" || header == "And") ? filters : stats;

			unsigned int num = Convert::ToLong(params);
			CombinerFilter::Ptr filter;

			if (header == "Or" || header == "StatsOr")
				filter = boost::make_shared<OrFilter>();
			else
				filter = boost::make_shared<AndFilter>();

			if (num > deq.size()) {
				m_Verb = "ERROR";
				m_ErrorCode = 451;
				m_ErrorMessage = "Or/StatsOr is referencing " + Convert::ToString(num) + " filters; stack only contains " + Convert::ToString(static_cast<long>(deq.size())) + " filters";
				return;
			}

			while (num-- && num > 0) {
				filter->AddSubFilter(deq.back());
				deq.pop_back();
			}

			deq.push_back(filter);
		} else if (header == "Negate" || header == "StatsNegate") {
			std::deque<Filter::Ptr>& deq = (header == "Negate") ? filters : stats;

			if (deq.empty()) {
				m_Verb = "ERROR";
				m_ErrorCode = 451;
				m_ErrorMessage = "Negate/StatsNegate used, however the filter stack is empty";
				return;
			}

			Filter::Ptr filter = deq.back();
			deq.pop_back();

			if (!filter) {
				m_Verb = "ERROR";
				m_ErrorCode = 451;
				m_ErrorMessage = "Negate/StatsNegate used, however last stats doesn't have a filter";
				return;
			}

			deq.push_back(boost::make_shared<NegateFilter>(filter));

			if (deq == stats) {
				Aggregator::Ptr aggregator = aggregators.back();
				aggregator->SetFilter(filter);
			}
		}
	}

	/* Combine all top-level filters into a single filter. */
	AndFilter::Ptr top_filter = boost::make_shared<AndFilter>();

	BOOST_FOREACH(const Filter::Ptr& filter, filters) {
		top_filter->AddSubFilter(filter);
	}

	m_Filter = top_filter;
	m_Aggregators.swap(aggregators);
}

int Query::GetExternalCommands(void)
{
	boost::mutex::scoped_lock lock(l_QueryMutex);

	return l_ExternalCommands;
}

Filter::Ptr Query::ParseFilter(const String& params)
{
	std::vector<String> tokens;
	boost::algorithm::split(tokens, params, boost::is_any_of(" "));

	if (tokens.size() == 2)
		tokens.push_back("");

	if (tokens.size() < 3)
		return Filter::Ptr();

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

	return filter;
}

void Query::PrintResultSet(std::ostream& fp, const std::vector<String>& columns, const Array::Ptr& rs)
{
	if (m_OutputFormat == "csv" && m_Columns.size() == 0 && m_ColumnHeaders) {
		bool first = true;

		BOOST_FOREACH(const String& column, columns) {
			if (first)
				first = false;
			else
				fp << m_Separators[1];

			fp << column;
		}

		fp << m_Separators[0];
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
					fp << m_Separators[1];

				if (value.IsObjectType<Array>())
					PrintCsvArray(fp, value, 0);
				else
					fp << value;
			}

			fp << m_Separators[0];
		}
	} else if (m_OutputFormat == "json") {
		fp << Value(rs).Serialize();
	}
}

void Query::PrintCsvArray(std::ostream& fp, const Array::Ptr& array, int level)
{
	bool first = true;

	ObjectLock olock(array);
	BOOST_FOREACH(const Value& value, array) {
		if (first)
			first = false;
		else
			fp << ((level == 0) ? m_Separators[2] : m_Separators[3]);

		if (value.IsObjectType<Array>())
			PrintCsvArray(fp, value, level + 1);
		else
			fp << value;
	}
}

void Query::ExecuteGetHelper(const Stream::Ptr& stream)
{
	Log(LogInformation, "livestatus", "Table: " + m_Table);

	Table::Ptr table = Table::GetByName(m_Table);

	if (!table) {
		SendResponse(stream, LivestatusErrorNotFound, "Table '" + m_Table + "' does not exist.");

		return;
	}

	std::vector<Value> objects = table->FilterRows(m_Filter);
	std::vector<String> columns;
	
	if (m_Columns.size() > 0)
		columns = m_Columns;
	else
		columns = table->GetColumnNames();

	Array::Ptr rs = boost::make_shared<Array>();

	if (m_Aggregators.empty()) {
		BOOST_FOREACH(const Value& object, objects) {
			Array::Ptr row = boost::make_shared<Array>();

			BOOST_FOREACH(const String& columnName, columns) {
				Column column = table->GetColumn(columnName);

				row->Add(column.ExtractValue(object));
			}

			rs->Add(row);
		}
	} else {
		std::vector<double> stats(m_Aggregators.size(), 0);

		int index = 0;
		BOOST_FOREACH(const Aggregator::Ptr aggregator, m_Aggregators) {
			BOOST_FOREACH(const Value& object, objects) {
				aggregator->Apply(table, object);
			}
			
			stats[index] = aggregator->GetResult();
			index++;
		}

		Array::Ptr row = boost::make_shared<Array>();
		for (size_t i = 0; i < m_Aggregators.size(); i++)
			row->Add(stats[i]);

		rs->Add(row);

		m_ColumnHeaders = false;
	}

	std::ostringstream result;
	PrintResultSet(result, columns, rs);

	SendResponse(stream, LivestatusErrorOK, result.str());
}

void Query::ExecuteCommandHelper(const Stream::Ptr& stream)
{
	{
		boost::mutex::scoped_lock lock(l_QueryMutex);

		l_ExternalCommands++;
	}

	Log(LogInformation, "livestatus", "Executing command: " + m_Command);
	ExternalCommandProcessor::Execute(m_Command);
	SendResponse(stream, LivestatusErrorOK, "");
}

void Query::ExecuteErrorHelper(const Stream::Ptr& stream)
{
	SendResponse(stream, m_ErrorCode, m_ErrorMessage);
}

void Query::SendResponse(const Stream::Ptr& stream, int code, const String& data)
{
	if (m_ResponseHeader == "fixed16")
		PrintFixed16(stream, code, data);

	if (m_ResponseHeader == "fixed16" || code == LivestatusErrorOK)
		stream->Write(data.CStr(), data.GetLength());
}

void Query::PrintFixed16(const Stream::Ptr& stream, int code, const String& data)
{
	ASSERT(code >= 100 && code <= 999);

	String sCode = Convert::ToString(code);
	String sLength = Convert::ToString(static_cast<long>(data.GetLength()));

	String header = sCode + String(16 - 3 - sLength.GetLength() - 1, ' ') + sLength + m_Separators[0];
	stream->Write(header.CStr(), header.GetLength());
}

bool Query::Execute(const Stream::Ptr& stream)
{
	try {
		Log(LogInformation, "livestatus", "Executing livestatus query: " + m_Verb);

		if (m_Verb == "GET")
			ExecuteGetHelper(stream);
		else if (m_Verb == "COMMAND")
			ExecuteCommandHelper(stream);
		else if (m_Verb == "ERROR")
			ExecuteErrorHelper(stream);
		else
			BOOST_THROW_EXCEPTION(std::runtime_error("Invalid livestatus query verb."));
	} catch (const std::exception& ex) {
		StackTrace *st = Exception::GetLastStackTrace();
		std::ostringstream info;
		st->Print(info);
		Log(LogDebug, "livestatus", info.str());
		SendResponse(stream, LivestatusErrorQuery, boost::diagnostic_information(ex));
	}

	if (!m_KeepAlive) {
		stream->Close();
		return false;
	}

	return true;
}
