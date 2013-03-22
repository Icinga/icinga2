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

#include "icinga/api.h"
#include "base/scriptfunction.h"
#include "base/logger_fwd.h"

using namespace icinga;

REGISTER_SCRIPTFUNCTION(GetAnswerToEverything, &API::GetAnswerToEverything);

void API::GetAnswerToEverything(const ScriptTask::Ptr& task, const std::vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Text argument required."));

	String text = arguments[0];

	Log(LogInformation, "icinga", "Hello from the Icinga 2 API: " + text);

	task->FinishResult(42);
}
