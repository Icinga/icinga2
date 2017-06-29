/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef GRAPHITEWRITER_H
#define GRAPHITEWRITER_H

#include "perfdata/graphitewriter.thpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/tcpsocket.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga graphite writer.
 *
 * @ingroup perfdata
 */
class GraphiteWriter : public ObjectImpl<GraphiteWriter>
{
public:
	DECLARE_OBJECT(GraphiteWriter);
	DECLARE_OBJECTNAME(GraphiteWriter);

	GraphiteWriter(void);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	virtual void ValidateHostNameTemplate(const String& value, const ValidationUtils& utils) override;
	virtual void ValidateServiceNameTemplate(const String& value, const ValidationUtils& utils) override;

protected:
	virtual void OnConfigLoaded(void) override;
	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;

private:
	Stream::Ptr m_Stream;
	WorkQueue m_WorkQueue;

	Timer::Ptr m_ReconnectTimer;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void CheckResultHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void SendMetric(const String& prefix, const String& name, double value, double ts);
	void SendPerfdata(const String& prefix, const CheckResult::Ptr& cr, double ts);
	static String EscapeMetric(const String& str, bool legacyMode = false);
	static String EscapeMetricLabel(const String& str);
	static Value EscapeMacroMetric(const Value& value, bool legacyMode = false);

	void ReconnectTimerHandler(void);

	void Disconnect(void);
	void Reconnect(void);

	void AssertOnWorkQueue(void);

	void ExceptionHandler(boost::exception_ptr exp);
};

}

#endif /* GRAPHITEWRITER_H */
