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

#include "livestatus/livestatusquery.hpp"
#include "livestatus/countaggregator.hpp"
#include "livestatus/sumaggregator.hpp"
#include "livestatus/minaggregator.hpp"
#include "livestatus/maxaggregator.hpp"
#include "livestatus/avgaggregator.hpp"
#include "livestatus/stdaggregator.hpp"
#include "livestatus/invsumaggregator.hpp"
#include "livestatus/invavgaggregator.hpp"
#include "livestatus/attributefilter.hpp"
#include "livestatus/negatefilter.hpp"
#include "livestatus/orfilter.hpp"
#include "livestatus/andfilter.hpp"
#include "icinga/externalcommandprocessor.hpp"
#include "base/debug.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/utility.hpp"
#include "base/json.hpp"
#include "base/serializer.hpp"
#include "base/timer.hpp"
#include "base/initialize.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

static int l_ExternalCommands = 0;
static boost::mutex l_QueryMutex;

LivestatusQuery::LivestatusQuery(const std::vector<String>& lines, const String& compat_log_path)
	: m_KeepAlive(false), m_OutputFormat("csv"), m_ColumnHeaders(true), m_Limit(-1), m_ErrorCode(0),
	m_LogTimeFrom(0), m_LogTimeUntil(static_cast<long>(Utility::GetTime()))
{
	if (lines.size() == 0) {
		m_Verb = "ERROR";
		m_ErrorCode = LivestatusErrorQuery;
		m_ErrorMessage = "Empty Query. Aborting.";
		return;
	}

	String msg;
	for (const String& line : lines) {
		msg += line + "\n";
	}
	Log(LogDebug, "LivestatusQuery", msg);

	m_CompatLogPath = compat_log_path;

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
		m_KeepAlive = true;
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

		//OutputFormat:json or OutputFormat: json
		if (line.GetLength() > col_index + 1)
			params = line.SubStr(col_index + 1).Trim();

		if (header == "ResponseHeader")
			m_ResponseHeader = params;
		else if (header == "OutputFormat")
			m_OutputFormat = params;
		else if (header == "KeepAlive")
			m_KeepAlive = (params == "on");
		else if (header == "Columns") {
			m_ColumnHeaders = false; // Might be explicitly re-enabled later on
			boost::algorithm::split(m_Columns, params, boost::is_any_of(" "));
		} else if (header == "Separators") {
			std::vector<String> separators;

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
		} else if (header == "ColumnHeaders")
			m_ColumnHeaders = (params == "on");
		else if (header == "Limit")
			m_Limit = Convert::ToLong(params);
		else if (header == "Filter") {
			Filter::Ptr filter = ParseFilter(params, m_LogTimeFrom, m_LogTimeUntil);

			if (!filter) {
				m_Verb = "ERROR";
				m_ErrorCode = LivestatusErrorQuery;
				m_ErrorMessage = "Invalid filter specification: " + line;
				return;
			}

			filters.push_back(filter);
		} else if (header == "Stats") {
			m_ColumnHeaders = false; // Might be explicitly re-enabled later on

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
				aggregator = new SumAggregator(aggregate_attr);
			} else if (aggregate_arg == "min") {
				aggregator = new MinAggregator(aggregate_attr);
			} else if (aggregate_arg == "max") {
				aggregator = new MaxAggregator(aggregate_attr);
			} else if (aggregate_arg == "avg") {
				aggregator = new AvgAggregator(aggregate_attr);
			} else if (aggregate_arg == "std") {
				aggregator = new StdAggregator(aggregate_attr);
			} else if (aggregate_arg == "suminv") {
				aggregator = new InvSumAggregator(aggregate_attr);
			} else if (aggregate_arg == "avginv") {
				aggregator = new InvAvgAggregator(aggregate_attr);
			} else {
				filter = ParseFilter(params, m_LogTimeFrom, m_LogTimeUntil);

				if (!filter) {
					m_Verb = "ERROR";
					m_ErrorCode = LivestatusErrorQuery;
					m_ErrorMessage = "Invalid filter specification: " + line;
					return;
				}

				aggregator = new CountAggregator();
			}

			aggregator->SetFilter(filter);
			aggregators.push_back(aggregator);

			stats.push_back(filter);
		} else if (header == "Or" || header == "And" || header == "StatsOr" || header == "StatsAnd") {
			std::deque<Filter::Ptr>& deq = (header == "Or" || header == "And") ? filters : stats;

			unsigned int num = Convert::ToLong(params);
			CombinerFilter::Ptr filter;

			if (header == "Or" || header == "StatsOr") {
				filter = new OrFilter();
				Log(LogDebug, "LivestatusQuery")
					<< "Add OR filter for " << params << " column(s). " << deq.size() << " filters available.";
			} else {
				filter = new AndFilter();
				Log(LogDebug, "LivestatusQuery")
					<< "Add AND filter for " << params << " column(s). " << deq.size() << " filters available.";
			}

			if (num > deq.size()) {
				m_Verb = "ERROR";
				m_ErrorCode = 451;
				m_ErrorMessage = "Or/StatsOr is referencing " + Convert::ToString(num) + " filters; stack only contains " + Convert::ToString(static_cast<long>(deq.size())) + " filters";
				return;
			}

			while (num > 0 && num--) {
				filter->AddSubFilter(deq.back());
				Log(LogDebug, "LivestatusQuery")
					<< "Add " << num << " filter.";
				deq.pop_back();
				if (&deq == &stats)
					aggregators.pop_back();
			}

			deq.push_back(filter);
			if (&deq == &stats) {
				Aggregator::Ptr aggregator = new CountAggregator();
				aggregator->SetFilter(filter);
				aggregators.push_back(aggregator);
			}
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

			deq.push_back(new NegateFilter(filter));

			if (deq == stats) {
				Aggregator::Ptr aggregator = aggregators.back();
				aggregator->SetFilter(filter);
			}
		}
	}

	/* Combine all top-level filters into a single filter. */
	AndFilter::Ptr top_filter = new AndFilter();

	for (const Filter::Ptr& filter : filters) {
		top_filter->AddSubFilter(filter);
	}

	m_Filter = top_filter;
	m_Aggregators.swap(aggregators);
}

int LivestatusQuery::GetExternalCommands()
{
	boost::mutex::scoped_lock lock(l_QueryMutex);

	return l_ExternalCommands;
}

Filter::Ptr LivestatusQuery::ParseFilter(const String& params, unsigned long& from, unsigned long& until)
{
	/*
	 * time >= 1382696656
	 * type = SERVICE FLAPPING ALERT
	 */
	std::vector<String> tokens;
	size_t sp_index;
	String temp_buffer = params;

	/* extract attr and op */
	for (int i = 0; i < 2; i++) {
		sp_index = temp_buffer.FindFirstOf(" ");

		/* check if this is the last argument */
		if (sp_index == String::NPos) {
			/* 'attr op' or 'attr op val' is valid */
			if (i < 1)
				BOOST_THROW_EXCEPTION(std::runtime_error("Livestatus filter '" + params + "' does not contain all required fields."));

			break;
		}

		tokens.emplace_back(temp_buffer.SubStr(0, sp_index));
		temp_buffer = temp_buffer.SubStr(sp_index + 1);
	}

	/* add the rest as value */
	tokens.emplace_back(std::move(temp_buffer));

	if (tokens.size() == 2)
		tokens.push_back("");

	if (tokens.size() < 3)
		return nullptr;

	bool negate = false;
	String attr = tokens[0];
	String op = tokens[1];
	String val = tokens[2];

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

	Filter::Ptr filter = new AttributeFilter(attr, op, val);

	if (negate)
		filter = new NegateFilter(filter);

	/* pre-filter log time duration */
	if (attr == "time") {
		if (op == "<" || op == "<=") {
			until = Convert::ToLong(val);
		} else if (op == ">" || op == ">=") {
			from = Convert::ToLong(val);
		}
	}

	Log(LogDebug, "LivestatusQuery")
		<< "Parsed filter with attr: '" << attr << "' op: '" << op << "' val: '" << val << "'.";

	return filter;
}

void LivestatusQuery::BeginResultSet(std::ostream& fp) const
{
	if (m_OutputFormat == "json" || m_OutputFormat == "python")
		fp << "[";
}

void LivestatusQuery::EndResultSet(std::ostream& fp) const
{
	if (m_OutputFormat == "json" || m_OutputFormat == "python")
		fp << "]";
}

void LivestatusQuery::AppendResultRow(std::ostream& fp, const Array::Ptr& row, bool& first_row) const
{
	if (m_OutputFormat == "csv") {
		bool first = true;

		ObjectLock rlock(row);
		for (const Value& value : row) {
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
	} else if (m_OutputFormat == "json") {
		if (!first_row)
			fp << ", ";

		fp << JsonEncode(row);
	} else if (m_OutputFormat == "python") {
		if (!first_row)
			fp << ", ";

		PrintPythonArray(fp, row);
	}

	first_row = false;
}

void LivestatusQuery::PrintCsvArray(std::ostream& fp, const Array::Ptr& array, int level) const
{
	bool first = true;

	ObjectLock olock(array);
	for (const Value& value : array) {
		if (first)
			first = false;
		else
			fp << ((level == 0) ? m_Separators[2] : m_Separators[3]);

		if (value.IsObjectType<Array>())
			PrintCsvArray(fp, value, level + 1);
		else if (value.IsBoolean())
			fp << Convert::ToLong(value);
		else
			fp << value;
	}
}

void LivestatusQuery::PrintPythonArray(std::ostream& fp, const Array::Ptr& rs) const
{
	fp << "[ ";

	bool first = true;

	for (const Value& value : rs) {
		if (first)
			first = false;
		else
			fp << ", ";

		if (value.IsObjectType<Array>())
			PrintPythonArray(fp, value);
		else if (value.IsNumber())
			fp << value;
		else
			fp << QuoteStringPython(value);
	}

	fp << " ]";
}

String LivestatusQuery::QuoteStringPython(const String& str) {
	String result = str;
	boost::algorithm::replace_all(result, "\"", "\\\"");
	return "r\"" + result + "\"";
}

void LivestatusQuery::ExecuteGetHelper(const Stream::Ptr& stream)
{
	Log(LogNotice, "LivestatusQuery")
		<< "Table: " << m_Table;

	Table::Ptr table = Table::GetByName(m_Table, m_CompatLogPath, m_LogTimeFrom, m_LogTimeUntil);

	if (!table) {
		SendResponse(stream, LivestatusErrorNotFound, "Table '" + m_Table + "' does not exist.");

		return;
	}

	std::vector<LivestatusRowValue> objects = table->FilterRows(m_Filter, m_Limit);
	std::vector<String> columns;

	if (m_Columns.size() > 0)
		columns = m_Columns;
	else
		columns = table->GetColumnNames();

	std::ostringstream result;
	bool first_row = true;
	BeginResultSet(result);

	if (m_Aggregators.empty()) {
		Array::Ptr header = new Array();

		typedef std::pair<String, Column> ColumnPair;

		std::vector<ColumnPair> column_objs;
		column_objs.reserve(columns.size());

		for (const String& columnName : columns)
			column_objs.emplace_back(columnName, table->GetColumn(columnName));

		for (const LivestatusRowValue& object : objects) {
			Array::Ptr row = new Array();

			row->Reserve(column_objs.size());

			for (const ColumnPair& cv : column_objs) {
				if (m_ColumnHeaders)
					header->Add(cv.first);

				row->Add(cv.second.ExtractValue(object.Row, object.GroupByType, object.GroupByObject));
			}

			if (m_ColumnHeaders) {
				AppendResultRow(result, header, first_row);
				m_ColumnHeaders = false;
			}

			AppendResultRow(result, row, first_row);
		}
	} else {
		std::map<std::vector<Value>, std::vector<AggregatorState *> > allStats;

		/* add aggregated stats */
		for (const LivestatusRowValue& object : objects) {
			std::vector<Value> statsKey;

			for (const String& columnName : m_Columns) {
				Column column = table->GetColumn(columnName);
				statsKey.emplace_back(column.ExtractValue(object.Row, object.GroupByType, object.GroupByObject));
			}

			auto it = allStats.find(statsKey);

			if (it == allStats.end()) {
				std::vector<AggregatorState *> newStats(m_Aggregators.size(), nullptr);
				it = allStats.insert(std::make_pair(statsKey, newStats)).first;
			}

			auto& stats = it->second;

			int index = 0;

			for (const Aggregator::Ptr aggregator : m_Aggregators) {
				aggregator->Apply(table, object.Row, &stats[index]);
				index++;
			}
		}

		/* add column headers both for raw and aggregated data */
		if (m_ColumnHeaders) {
			Array::Ptr header = new Array();

			for (const String& columnName : m_Columns) {
				header->Add(columnName);
			}

			for (size_t i = 1; i <= m_Aggregators.size(); i++) {
				header->Add("stats_" + Convert::ToString(i));
			}

			AppendResultRow(result, header, first_row);
		}

		for (const auto& kv : allStats) {
			Array::Ptr row = new Array();

			row->Reserve(m_Columns.size() + m_Aggregators.size());

			for (const Value& keyPart : kv.first) {
				row->Add(keyPart);
			}

			auto& stats = kv.second;

			for (size_t i = 0; i < m_Aggregators.size(); i++)
				row->Add(m_Aggregators[i]->GetResultAndFreeState(stats[i]));

			AppendResultRow(result, row, first_row);
		}

		/* add a bogus zero value if aggregated is empty*/
		if (allStats.empty()) {
			Array::Ptr row = new Array();

			for (size_t i = 1; i <= m_Aggregators.size(); i++) {
				row->Add(0);
			}

			AppendResultRow(result, row, first_row);
		}
	}

	EndResultSet(result);

	SendResponse(stream, LivestatusErrorOK, result.str());
}

void LivestatusQuery::ExecuteCommandHelper(const Stream::Ptr& stream)
{
	{
		boost::mutex::scoped_lock lock(l_QueryMutex);

		l_ExternalCommands++;
	}

	Log(LogNotice, "LivestatusQuery")
		<< "Executing command: " << m_Command;
	ExternalCommandProcessor::Execute(m_Command);
	SendResponse(stream, LivestatusErrorOK, "");
}

void LivestatusQuery::ExecuteErrorHelper(const Stream::Ptr& stream)
{
	Log(LogDebug, "LivestatusQuery")
		<< "ERROR: Code: '" << m_ErrorCode << "' Message: '" << m_ErrorMessage << "'.";
	SendResponse(stream, m_ErrorCode, m_ErrorMessage);
}

void LivestatusQuery::SendResponse(const Stream::Ptr& stream, int code, const String& data)
{
	if (m_ResponseHeader == "fixed16")
		PrintFixed16(stream, code, data);

	if (m_ResponseHeader == "fixed16" || code == LivestatusErrorOK) {
		try {
			stream->Write(data.CStr(), data.GetLength());
		} catch (const std::exception&) {
			Log(LogCritical, "LivestatusQuery", "Cannot write query response to socket.");
		}
	}
}

void LivestatusQuery::PrintFixed16(const Stream::Ptr& stream, int code, const String& data)
{
	ASSERT(code >= 100 && code <= 999);

	String sCode = Convert::ToString(code);
	String sLength = Convert::ToString(static_cast<long>(data.GetLength()));

	String header = sCode + String(16 - 3 - sLength.GetLength() - 1, ' ') + sLength + m_Separators[0];

	try {
		stream->Write(header.CStr(), header.GetLength());
	} catch (const std::exception&) {
		Log(LogCritical, "LivestatusQuery", "Cannot write to TCP socket.");
	}
}

bool LivestatusQuery::Execute(const Stream::Ptr& stream)
{
	try {
		Log(LogNotice, "LivestatusQuery")
			<< "Executing livestatus query: " << m_Verb;

		if (m_Verb == "GET")
			ExecuteGetHelper(stream);
		else if (m_Verb == "COMMAND")
			ExecuteCommandHelper(stream);
		else if (m_Verb == "ERROR")
			ExecuteErrorHelper(stream);
		else
			BOOST_THROW_EXCEPTION(std::runtime_error("Invalid livestatus query verb."));
	} catch (const std::exception& ex) {
		SendResponse(stream, LivestatusErrorQuery, DiagnosticInformation(ex));
	}

	if (!m_KeepAlive) {
		stream->Close();
		return false;
	}

	return true;
}
