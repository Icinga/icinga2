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

#ifndef INFLUXDBWRITER_H
#define INFLUXDBWRITER_H

#include "perfdata/influxdbwriter.thpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/tcpsocket.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga InfluxDB writer.
 *
 * @ingroup perfdata
 */
class InfluxdbWriter final : public ObjectImpl<InfluxdbWriter>
{
public:
	DECLARE_OBJECT(InfluxdbWriter);
	DECLARE_OBJECTNAME(InfluxdbWriter);

	InfluxdbWriter();

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	virtual void ValidateHostTemplate(const Dictionary::Ptr& value, const ValidationUtils& utils) override;
	virtual void ValidateServiceTemplate(const Dictionary::Ptr& value, const ValidationUtils& utils) override;

protected:
	virtual void OnConfigLoaded() override;
	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;

private:
	WorkQueue m_WorkQueue;
	Timer::Ptr m_FlushTimer;
	std::vector<String> m_DataBuffer;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void CheckResultHandlerWQ(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void SendMetric(const Dictionary::Ptr& tmpl, const String& label, const Dictionary::Ptr& fields, double ts);
	void FlushTimeout();
	void FlushTimeoutWQ();
	void Flush();

	static String EscapeKeyOrTagValue(const String& str);
	static String EscapeValue(const Value& value);

	Stream::Ptr Connect();

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);
};

}

#endif /* INFLUXDBWRITER_H */
