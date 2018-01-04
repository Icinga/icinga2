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

#ifndef OPENTSDBWRITER_H
#define OPENTSDBWRITER_H

#include "perfdata/opentsdbwriter.thpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/tcpsocket.hpp"
#include "base/timer.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga opentsdb writer.
 *
 * @ingroup perfdata
 */
class OpenTsdbWriter final : public ObjectImpl<OpenTsdbWriter>
{
public:
	DECLARE_OBJECT(OpenTsdbWriter);
	DECLARE_OBJECTNAME(OpenTsdbWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	Stream::Ptr m_Stream;

	Timer::Ptr m_ReconnectTimer;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void SendMetric(const String& metric, const std::map<String, String>& tags, double value, double ts);
	void SendPerfdata(const String& metric, const std::map<String, String>& tags, const CheckResult::Ptr& cr, double ts);
	static String EscapeTag(const String& str);
	static String EscapeMetric(const String& str);

	void ReconnectTimerHandler();
};

}

#endif /* OPENTSDBWRITER_H */
