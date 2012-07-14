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

#include "i2-cib.h"

using namespace icinga;

map<string, CheckTaskType> CheckTask::m_Types;

CheckTask::CheckTask(const Service& service, const CompletionCallback& completionCallback)
	: AsyncTask(completionCallback), m_Service(service)
{ }

Service& CheckTask::GetService(void)
{
	return m_Service;
}

CheckResult& CheckTask::GetResult(void)
{
	return m_Result;
}

void CheckTask::RegisterType(string type, Factory factory)
{
	CheckTaskType ctt;
	ctt.Factory = factory;

	m_Types[type] = ctt;
}

CheckTask::Ptr CheckTask::CreateTask(const Service& service, const CompletionCallback& completionCallback)
{
	map<string, CheckTaskType>::iterator it;

	it = m_Types.find(service.GetCheckType());

	if (it == m_Types.end())
		throw runtime_error("Invalid check type specified for service '" + service.GetName() + "'");

	return it->second.Factory(service, completionCallback);
}
