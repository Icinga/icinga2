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
	static Dictionary::Ptr SendCustomNotification(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr DelayNotification(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr AcknowledgeProblem(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RemoveAcknowledgement(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr AddComment(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RemoveComment(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ScheduleDowntime(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RemoveDowntime(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr ShutdownProcess(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr RestartProcess(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);
	static Dictionary::Ptr GenerateTicket(const ConfigObject::Ptr& object, const Dictionary::Ptr& params);

private:
	static Dictionary::Ptr CreateResult(int code, const String& status, const Dictionary::Ptr& additional = nullptr);
};

}

#endif /* APIACTIONS_H */
