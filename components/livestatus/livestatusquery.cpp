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



LivestatusQuery::LivestatusQuery(const vector<String>& lines)
	: m_KeepAlive(false), m_ColumnHeaders(true), m_Limit(-1)
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
		BOOST_THROW_EXCEPTION(runtime_error("Unknown livestatus verb: " + m_Verb));
	}

	for (int i = 1; i < lines.size(); i++) {
		line = lines[i];

		size_t col_index = line.FindFirstOf(":");
		String header = line.SubStr(0, col_index);
		String params = line.SubStr(col_index + 2);

		if (header == "ResponseHeader")
			m_ResponseHeader = params;
	}
}

void LivestatusQuery::ExecuteGetHelper(const Stream::Ptr& stream)
{
	Logger::Write(LogInformation, "livestatus", "Table: " + m_Table);
}

void LivestatusQuery::ExecuteCommandHelper(const Stream::Ptr& stream)
{
	try {
		Logger::Write(LogInformation, "livestatus", "Executing command: " + m_Command);
		ExternalCommandProcessor::Execute(m_Command);
		SendResponse(stream, 200, "");
	} catch (const std::exception& ex) {
		SendResponse(stream, 452, diagnostic_information(ex));
	}
}

void LivestatusQuery::ExecuteErrorHelper(const Stream::Ptr& stream)
{
	PrintFixed16(stream, m_ErrorCode, m_ErrorMessage);
}

void LivestatusQuery::SendResponse(const Stream::Ptr& stream, int code, const String& data)
{
	if (m_ResponseHeader == "fixed16")
		PrintFixed16(stream, code, data);

	if (m_ResponseHeader == "fixed16" || code == 200)
		stream->Write(data.CStr(), data.GetLength());
}

void LivestatusQuery::PrintFixed16(const Stream::Ptr& stream, int code, const String& data)
{
	ASSERT(code >= 100 && code <= 999);

	String sCode = Convert::ToString(code);
	String sLength = Convert::ToString(data.GetLength());

	String header = sCode + String(16 - 3 - sLength.GetLength() - 1, ' ') + sLength + "\n";
	stream->Write(header.CStr(), header.GetLength());
}

void LivestatusQuery::Execute(const Stream::Ptr& stream)
{
	Logger::Write(LogInformation, "livestatus", "Executing livestatus query: " + m_Verb);

	if (m_Verb == "GET")
		ExecuteGetHelper(stream);
	else if (m_Verb == "COMMAND")
		ExecuteCommandHelper(stream);
	else if (m_Verb == "ERROR")
		ExecuteErrorHelper(stream);
	else
		BOOST_THROW_EXCEPTION(runtime_error("Invalid livestatus query verb."));

	if (!m_KeepAlive)
		stream->Close();
}
