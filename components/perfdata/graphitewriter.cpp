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

#include "perfdata/graphitewriter.h"
#include "icinga/service.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "icinga/compatutility.h"
#include "base/tcpsocket.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/application.h"
#include "base/stream.h"
#include "base/networkstream.h"
#include "base/bufferedstream.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/exception/diagnostic_information.hpp>

using namespace icinga;

REGISTER_TYPE(GraphiteWriter);

void GraphiteWriter::Start(void)
{
	DynamicObject::Start();

	m_ReconnectTimer = boost::make_shared<Timer>();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&GraphiteWriter::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	Service::OnNewCheckResult.connect(boost::bind(&GraphiteWriter::CheckResultHandler, this, _1, _2));
}

String GraphiteWriter::GetHost(void) const
{
	if (m_Host.IsEmpty())
		return "127.0.0.1";
	else
		return m_Host;
}

String GraphiteWriter::GetPort(void) const
{
	if (m_Port.IsEmpty())
		return "2003";
	else
		return m_Port;
}

void GraphiteWriter::ReconnectTimerHandler(void)
{
	try {
		if (m_Stream) {
			m_Stream->Write("\n", 1);
			Log(LogWarning, "perfdata", "GraphiteWriter already connected on socket on host '" + GetHost() + "' port '" + GetPort() + "'.");
			return;
		}
	} catch (const std::exception& ex) {
		Log(LogWarning, "perfdata", "GraphiteWriter socket on host '" + GetHost() + "' port '" + GetPort() + "' gone. Attempting to reconnect.");	
	}

	TcpSocket::Ptr socket = boost::make_shared<TcpSocket>();

	Log(LogDebug, "perfdata", "GraphiteWriter: Reconnect to tcp socket on host '" + GetHost() + "' port '" + GetPort() + "'.");
	socket->Connect(GetHost(), GetPort());

	NetworkStream::Ptr net_stream = boost::make_shared<NetworkStream>(socket);
	m_Stream = boost::make_shared<BufferedStream>(net_stream);
}

void GraphiteWriter::CheckResultHandler(const Service::Ptr& service, const Dictionary::Ptr& cr)
{
	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !service->GetEnablePerfdata())
		return;

	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	/* service metrics */
	std::vector<String> metrics;
	String metricName;
	Value metricValue;

	/* basic metrics */
	AddServiceMetric(metrics, service, "current_attempt", service->GetCurrentCheckAttempt());
	AddServiceMetric(metrics, service, "max_check_attempts", service->GetMaxCheckAttempts());
	AddServiceMetric(metrics, service, "state_type", service->GetStateType());
	AddServiceMetric(metrics, service, "state", service->GetState());
	AddServiceMetric(metrics, service, "latency", Service::CalculateLatency(cr));
	AddServiceMetric(metrics, service, "execution_time", Service::CalculateExecutionTime(cr));

	/* performance data metrics */
	String perfdata = CompatUtility::GetCheckResultPerfdata(cr);

	if (!perfdata.IsEmpty()) {
		perfdata.Trim();

		Log(LogDebug, "perfdata", "GraphiteWriter: Processing perfdata: '" + perfdata + "'.");

		/*
		 * 'foo bar'=0;;; baz=0.0;;;
		 * 'label'=value[UOM];[warn];[crit];[min];[max]
		*/
		std::vector<String> tokens;
		boost::algorithm::split(tokens, perfdata, boost::is_any_of(" "));

		/* TODO deal with white spaces in single quoted labels: 'foo bar'=0;;; 'baz'=1.0;;;
		 * 1. find first ', find second ' -> if no '=' in between, this is a label
		 * 2. two single quotes define an escaped single quite
		 * 3. warn/crit/min/max may be null and semicolon delimiter omitted
		 * https://www.nagios-plugins.org/doc/guidelines.html#AEN200
		 */
		BOOST_FOREACH(const String& token, tokens) {
			String metricKeyVal = token;
			metricKeyVal.Trim();

			std::vector<String> key_val;
			boost::algorithm::split(key_val, metricKeyVal, boost::is_any_of("="));

			if (key_val.size() == 0) {
				Log(LogWarning, "perfdata", "GraphiteWriter: Invalid performance data. No assignment operator found in :'" + metricKeyVal + "'.");
				return;
			}

			String metricName = key_val[0];
			metricName.Trim();

			if (key_val.size() == 1) {
				Log(LogWarning, "perfdata", "GraphiteWriter: Invalid performance data: '" + metricKeyVal + "' with key: '" + metricName + "'.");
				return;
			}

			String metricValues = key_val[1];
			metricValues.Trim();

			std::vector<String> perfdata_values;
			boost::algorithm::split(perfdata_values, metricValues, boost::is_any_of(";"));

			if (perfdata_values.size() == 0) {
				Log(LogWarning, "perfdata", "GraphiteWriter: Invalid performance data: '" + metricKeyVal +
				    "' with key: '" + metricName + "' and values: '" + metricValues + "'.");
				return;
			}

			String metricValue = perfdata_values[0];

			metricValue.Trim();
			Log(LogDebug, "perfdata", "GraphiteWriter: Trimmed metric value: '" + metricValue + "'.");

			/* extract raw value (digit number digit as double) and uom
			 * http://en.highscore.de/cpp/boost/stringhandling.html
			 */
			String metricValueRaw = boost::algorithm::trim_right_copy_if(metricValue, (!boost::algorithm::is_digit() && !boost::algorithm::is_any_of(".,")));
			String metricValueUom = boost::algorithm::trim_left_copy_if(metricValue, (boost::algorithm::is_digit() || boost::algorithm::is_any_of(".,")));

			Log(LogDebug, "perfdata", "GraphiteWriter: Raw metric value: '" + metricValueRaw + "' with UOM: '" + metricValueUom + "'.");

			/* TODO: Normalize raw value based on UOM
			 * a. empty - assume a number
			 * b. 's' - seconds (us, ms)
			 * c. '%' - percentage
			 * d. 'B' - bytes (KB, MB, GB, TB)
			 * e. 'c' - continous counter (snmp)
			 */

			/* //TODO: Figure out how graphite handles warn/crit/min/max
			String metricValueWarn, metricValueCrit, metricValueMin, metricValueMax;

			if (perfdata_values.size() > 1)
				metricValueWarn = perfdata_values[1];
			if (perfdata_values.size() > 2)
				metricValueCrit = perfdata_values[2];
			if (perfdata_values.size() > 3)
				metricValueMin = perfdata_values[3];
			if (perfdata_values.size() > 4)
				metricValueMax = perfdata_values[4];
			*/

			/* sanitize invalid metric characters */
			SanitizeMetric(metricName);

			AddServiceMetric(metrics, service, metricName, metricValueRaw);
		}
	}

	SendMetrics(metrics);
}

void GraphiteWriter::AddServiceMetric(std::vector<String>& metrics, const Service::Ptr& service, const String& name, const Value& value)
{
	/* TODO: sanitize host and service names */
	String hostName = service->GetHost()->GetName();
	String serviceName = service->GetShortName();	

	SanitizeMetric(hostName);
	SanitizeMetric(serviceName);

	String metricPrefix = hostName + "." + serviceName;
	String graphitePrefix = "icinga";

	String metric = graphitePrefix + "." + metricPrefix + "." + name + " " + Convert::ToString(value) + " " + Convert::ToString(static_cast<long>(Utility::GetTime())) + "\n";
	Log(LogDebug, "perfdata", "GraphiteWriter: Add to metric list:'" + metric + "'.");
	metrics.push_back(metric);
}

void GraphiteWriter::SendMetrics(const std::vector<String>& metrics)
{
	BOOST_FOREACH(const String& metric, metrics) {
		if (metric.IsEmpty())
			continue;

		Log(LogDebug, "perfdata", "GraphiteWriter: Sending metric '" + metric + "'.");

		ObjectLock olock(this);

		if (!m_Stream)
			return;

		try {
			m_Stream->Write(metric.CStr(), metric.GetLength());
		} catch (const std::exception& ex) {
			std::ostringstream msgbuf;
			msgbuf << "Exception thrown while writing to the Graphite socket: " << std::endl
                               << boost::diagnostic_information(ex);

			Log(LogCritical, "base", msgbuf.str());

			m_Stream.reset();
		}
	}
}

void GraphiteWriter::SanitizeMetric(String& str)
{
	boost::replace_all(str, " ", "_");
	boost::replace_all(str, ".", "_");
	boost::replace_all(str, "-", "_");
	boost::replace_all(str, "\\", "_");
	boost::replace_all(str, "/", "_");
}

void GraphiteWriter::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("host", m_Host);
		bag->Set("port", m_Port);
	}
}

void GraphiteWriter::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_Host = bag->Get("host");
		m_Port = bag->Get("port");
	}
}
