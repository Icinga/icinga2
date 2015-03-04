/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "config/configcompiler.hpp"
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
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

static int l_ExternalCommands = 0;
static boost::mutex l_QueryMutex;
static std::map<String, LivestatusScriptFrame> l_LivestatusScriptFrames;
static Timer::Ptr l_FrameCleanupTimer;
static boost::mutex l_LivestatusScriptMutex;

static void ScriptFrameCleanupHandler(void)
{
	boost::mutex::scoped_lock lock(l_LivestatusScriptMutex);

	std::vector<String> cleanup_keys;

	typedef std::pair<String, LivestatusScriptFrame> KVPair;

	BOOST_FOREACH(const KVPair& kv, l_LivestatusScriptFrames) {
		if (kv.second.Seen < Utility::GetTime() - 1800)
			cleanup_keys.push_back(kv.first);
	}

	BOOST_FOREACH(const String& key, cleanup_keys)
		l_LivestatusScriptFrames.erase(key);
}

static void InitScriptFrameCleanup(void)
{
	l_FrameCleanupTimer = new Timer();
	l_FrameCleanupTimer->OnTimerExpired.connect(boost::bind(ScriptFrameCleanupHandler));
	l_FrameCleanupTimer->SetInterval(30);
	l_FrameCleanupTimer->Start();
}

INITIALIZE_ONCE(InitScriptFrameCleanup);

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
	BOOST_FOREACH(const String& line, lines) {
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
	} else if (m_Verb == "SCRIPT") {
		m_Session = target;

		for (unsigned int i = 1; i < lines.size(); i++) {
			if (m_Command != "")
				m_Command += "\n";
			m_Command += lines[i];
		}

		return;
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
			params = line.SubStr(col_index + 1);

		params.Trim();

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

	BOOST_FOREACH(const Filter::Ptr& filter, filters) {
		top_filter->AddSubFilter(filter);
	}

	m_Filter = top_filter;
	m_Aggregators.swap(aggregators);
}

int LivestatusQuery::GetExternalCommands(void)
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

		tokens.push_back(temp_buffer.SubStr(0, sp_index));
		temp_buffer = temp_buffer.SubStr(sp_index + 1);
	}

	/* add the rest as value */
	tokens.push_back(temp_buffer);

	if (tokens.size() == 2)
		tokens.push_back("");

	if (tokens.size() < 3)
		return Filter::Ptr();

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

void LivestatusQuery::PrintResultSet(std::ostream& fp, const Array::Ptr& rs) const
{
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
		fp << JsonEncode(rs);
	} else if (m_OutputFormat == "python") {
		PrintPythonArray(fp, rs);
	}
}

void LivestatusQuery::PrintCsvArray(std::ostream& fp, const Array::Ptr& array, int level) const
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

	BOOST_FOREACH(const Value& value, rs) {
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

	Array::Ptr rs = new Array();

	if (m_Aggregators.empty()) {
		Array::Ptr header = new Array();

		typedef std::pair<String, Column> ColumnPair;

		std::vector<ColumnPair> column_objs;
		column_objs.reserve(columns.size());

		BOOST_FOREACH(const String& columnName, columns)
			column_objs.push_back(std::make_pair(columnName, table->GetColumn(columnName)));

		rs->Reserve(1 + objects.size());

		BOOST_FOREACH(const LivestatusRowValue& object, objects) {
			Array::Ptr row = new Array();

			row->Reserve(column_objs.size());

			BOOST_FOREACH(const ColumnPair& cv, column_objs) {
				if (m_ColumnHeaders)
					header->Add(cv.first);

				row->Add(cv.second.ExtractValue(object.Row, object.GroupByType, object.GroupByObject));
			}

			if (m_ColumnHeaders) {
				rs->Add(header);
				m_ColumnHeaders = false;
			}

			rs->Add(row);
		}
	} else {
		std::vector<double> stats(m_Aggregators.size(), 0);
		int index = 0;

		/* add aggregated stats */
		BOOST_FOREACH(const Aggregator::Ptr aggregator, m_Aggregators) {
			BOOST_FOREACH(const LivestatusRowValue& object, objects) {
				aggregator->Apply(table, object.Row);
			}

			stats[index] = aggregator->GetResult();
			index++;
		}

		/* add column headers both for raw and aggregated data */
		if (m_ColumnHeaders) {
			Array::Ptr header = new Array();

			BOOST_FOREACH(const String& columnName, m_Columns) {
				header->Add(columnName);
			}

			for (size_t i = 1; i <= m_Aggregators.size(); i++) {
				header->Add("stats_" + Convert::ToString(i));
			}

			rs->Add(header);
		}

		Array::Ptr row = new Array();

		row->Reserve(m_Columns.size() + m_Aggregators.size());

		/*
		 * add selected columns next to stats
		 * may not be accurate for grouping!
		 */
		if (objects.size() > 0 && m_Columns.size() > 0) {
			BOOST_FOREACH(const String& columnName, m_Columns) {
				Column column = table->GetColumn(columnName);

				LivestatusRowValue object = objects[0]; //first object wins

				row->Add(column.ExtractValue(object.Row, object.GroupByType, object.GroupByObject));
			}
		}

		for (size_t i = 0; i < m_Aggregators.size(); i++)
			row->Add(stats[i]);

		rs->Add(row);
	}

	std::ostringstream result;
	PrintResultSet(result, rs);

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

void LivestatusQuery::ExecuteScriptHelper(const Stream::Ptr& stream)
{
	Log(LogInformation, "LivestatusQuery")
	    << "Executing expression: " << m_Command;

	m_ResponseHeader = "fixed16";

	LivestatusScriptFrame& lsf = l_LivestatusScriptFrames[m_Session];
	lsf.Seen = Utility::GetTime();

	if (!lsf.Locals)
		lsf.Locals = new Dictionary();

	String fileName = "<" + Convert::ToString(lsf.NextLine) + ">";
	lsf.NextLine++;

	lsf.Lines[fileName] = m_Command;

	Expression *expr = ConfigCompiler::CompileText(fileName, m_Command);
	Value result;
	try {
		ScriptFrame frame;
		frame.Locals = lsf.Locals;
		result = expr->Evaluate(frame);
	} catch (const ScriptError& ex) {
		delete expr;

		DebugInfo di = ex.GetDebugInfo();

		std::ostringstream msgbuf;

		msgbuf << di.Path << ": " << lsf.Lines[di.Path] << "\n"
		    << String(di.Path.GetLength() + 2, ' ')
		    << String(di.FirstColumn, ' ') << String(di.LastColumn - di.FirstColumn + 1, '^') << "\n"
		    << ex.what() << "\n";

		SendResponse(stream, LivestatusErrorQuery, msgbuf.str());
		return;
	} catch (...) {
		delete expr;
		throw;
	}
	delete expr;
	SendResponse(stream, LivestatusErrorOK, JsonEncode(Serialize(result, FAState | FAConfig), true));
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
			Log(LogCritical, "LivestatusQuery", "Cannot write to TCP socket.");
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
		else if (m_Verb == "SCRIPT")
			ExecuteScriptHelper(stream);
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
