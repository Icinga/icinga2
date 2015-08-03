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

#ifndef APIACTIONS_H
#define APIACTIONS_H

#include "icinga/i2-icinga.hpp"
#include "base/configobject.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * @ingroup icinga
 */
class I2_ICINGA_API ApiActions
{
public:
	static Dictionary::Ptr ProcessCheckResult(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RescheduleCheck(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr DelayNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);

	static Dictionary::Ptr AcknowledgeProblem(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RemoveAcknowledgement(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr AddComment(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
/*	static Dictionary::Ptr RemoveComment(const ConfigObject::Ptr& object, const Dictionary::Ptr& params); */
	static Dictionary::Ptr RemoveAllComments(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ScheduleDowntime(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
/*	static Dictionary::Ptr RemoveDowntime(const ConfigObject::Ptr& object, const Dictionary::Ptr& params); */

	static Dictionary::Ptr EnablePassiveChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr DisablePassiveChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr EnableActiveChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr DisableActiveChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr EnableNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr DisableNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr EnableFlapDetection(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr DisableFlapDetection(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);

/*	static Dictionary::Ptr ChangeEventHandler(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ChangeCheckCommand(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ChangeMaxCheckAttempts(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ChangeCheckInterval(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ChangeRetryInterval(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ChangeCheckPeriod(const ConfigObject::Ptr& object, const Dictionary::Ptr& params); */

/*	static Dictionary::Ptr ApiActions::EnableNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::DisableNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::EnableFlapDetection(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::DisableFlapDetection(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::EnableEventHandlers(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::DisableEventHandlers(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::EnablePerformanceData(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::DisablePerformanceData(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::StartExecutingSvcChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::StopExecutingSvcChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::StartExecutingHostChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::StopExecutingHostChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params); */

/*
	static Dictionary::Ptr ApiActions::ShutdownProcess(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::RestartProcess(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ApiActions::ProcessFile(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	*/

private:
	static Dictionary::Ptr CreateResult(int code, const String& status);
};

}

#endif /* APIACTIONS_H */
 
