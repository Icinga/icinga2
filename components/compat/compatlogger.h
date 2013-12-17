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

#ifndef COMPATLOGGER_H
#define COMPATLOGGER_H

#include "compat/compatlogger.th"
#include "icinga/service.h"
#include "base/timer.h"
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
	DECLARE_PTR_TYPEDEFS(CompatLogger);
	DECLARE_TYPENAME(CompatLogger);

	static void ValidateRotationMethod(const String& location, const Dictionary::Ptr& attrs);

protected:
	virtual void Start(void);

private:
	void WriteLine(const String& line);
	void Flush(void);

	void CheckResultHandler(const Service::Ptr& service, const CheckResult::Ptr& cr);
	void NotificationSentHandler(const Service::Ptr& service, const User::Ptr& user, NotificationType const& notification_type, CheckResult::Ptr const& cr, const String& author, const String& comment_text, const String& command_name);
	void FlappingHandler(const Service::Ptr& service, FlappingState flapping_state);
	void TriggerDowntimeHandler(const Service::Ptr& service, const Downtime::Ptr& downtime);
	void RemoveDowntimeHandler(const Service::Ptr& service, const Downtime::Ptr& downtime);
	void ExternalCommandHandler(const String& command, const std::vector<String>& arguments);
	void EventCommandHandler(const Service::Ptr& service);

	Timer::Ptr m_RotationTimer;
	void RotationTimerHandler(void);
	void ScheduleNextRotation(void);

	std::ofstream m_OutputFile;
	void ReopenFile(bool rotate);
};

}

#endif /* COMPATLOGGER_H */
