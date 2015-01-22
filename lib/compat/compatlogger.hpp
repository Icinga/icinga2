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

#ifndef COMPATLOGGER_H
#define COMPATLOGGER_H

#include "compat/compatlogger.thpp"
#include "icinga/service.hpp"
#include "base/timer.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga compat log writer.
 *
 * @ingroup compat
 */
class CompatLogger : public ObjectImpl<CompatLogger>
{
public:
	DECLARE_OBJECT(CompatLogger);
	DECLARE_OBJECTNAME(CompatLogger);

	static Value StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static void ValidateRotationMethod(const String& location, const Dictionary::Ptr& attrs);

protected:
	virtual void Start(void);

private:
	void WriteLine(const String& line);
	void Flush(void);

	void CheckResultHandler(const Checkable::Ptr& service, const CheckResult::Ptr& cr);
	void NotificationSentHandler(const Notification::Ptr& notification, const Checkable::Ptr& service,
	    const User::Ptr& user, NotificationType notification_type, CheckResult::Ptr const& cr,
	    const String& author, const String& comment_text, const String& command_name);
	void FlappingHandler(const Checkable::Ptr& service, FlappingState flapping_state);
	void TriggerDowntimeHandler(const Checkable::Ptr& service, const Downtime::Ptr& downtime);
	void RemoveDowntimeHandler(const Checkable::Ptr& service, const Downtime::Ptr& downtime);
	void ExternalCommandHandler(const String& command, const std::vector<String>& arguments);
	void EventCommandHandler(const Checkable::Ptr& service);

	Timer::Ptr m_RotationTimer;
	void RotationTimerHandler(void);
	void ScheduleNextRotation(void);

	std::ofstream m_OutputFile;
	void ReopenFile(bool rotate);
};

}

#endif /* COMPATLOGGER_H */
