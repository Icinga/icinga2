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

int CIB::m_Types;
RingBuffer CIB::m_TaskStatistics(15 * 60);
boost::signal<void (const ServiceStatusMessage&)> CIB::OnServiceStatusUpdate;

void CIB::RequireInformation(InformationType types)
{
	m_Types |= types;

	Application::Ptr app = Application::GetInstance();
	Component::Ptr component = app->GetComponent("cibsync");

	if (!component) {
		ConfigItemBuilder::Ptr cb = boost::make_shared<ConfigItemBuilder>();
		cb->SetType("component");
		cb->SetName("cibsync");
		cb->SetLocal(true);
		ConfigItem::Ptr ci = cb->Compile();
		ci->Commit();
	}
}

void CIB::UpdateTaskStatistics(long tv, int num)
{
	m_TaskStatistics.InsertValue(tv, num);
}

int CIB::GetTaskStatistics(long timespan)
{
	return m_TaskStatistics.GetValues(timespan);
}

int CIB::GetInformationTypes(void)
{
	return m_Types;
}

