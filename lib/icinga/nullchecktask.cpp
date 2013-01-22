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

#include "i2-icinga.h"

using namespace icinga;

REGISTER_SCRIPTFUNCTION("native::NullCheck",  &NullCheckTask::ScriptFunc);

void NullCheckTask::ScriptFunc(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Missing argument: Service must be specified."));

	double now = Utility::GetTime();

	Dictionary::Ptr cr = boost::make_shared<Dictionary>();
	cr->Set("state", StateUnknown);

	task->FinishResult(cr);
}
