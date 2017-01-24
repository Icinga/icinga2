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

#ifndef LOGSTASHWRITER_H
#define LOGSTASHWRITER_H

#include "perfdata/logstashwriter.thpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/tcpsocket.hpp"
#include "base/udpsocket.hpp"
#include "base/timer.hpp"
#include <fstream>
#include <string>

namespace icinga
{

/**
 * An Icinga logstash writer.
 *
 * @ingroup perfdata
 */
class LogstashWriter : public ObjectImpl<LogstashWriter>
{

public:
	DECLARE_OBJECT(LogstashWriter);
	DECLARE_OBJECTNAME(LogstashWriter);

	virtual void ValidateSocketType(const String& value, const ValidationUtils& utils) override;

protected:
	virtual void Start(bool runtimeCreated) override;

private:
	Stream::Ptr m_Stream;

	Timer::Ptr m_ReconnectTimer;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void NotificationToUserHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
	const User::Ptr& user, NotificationType notification_type, CheckResult::Ptr const& cr,
	const String& author, const String& comment_text, const String& command_name);
	void StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type);
	void SendLogMessage(const String& message);
	String ComposeLogstashMessage(const Dictionary::Ptr& fields, const String& source, double ts);

	static String EscapeMetricLabel(const String& str);

	void ReconnectTimerHandler(void);
};

}

#endif /* LOGSTASHWRITER_H */
