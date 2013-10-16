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

#include "icinga/service.h"
#include "icinga/eventcommand.h"
#include "icinga/icingaapplication.h"

using namespace icinga;

boost::signals2::signal<void (const Service::Ptr&)> Service::OnEventCommandExecuted;

bool Service::GetEnableEventHandler(void) const
{
	if (!m_OverrideEnableEventHandler.IsEmpty())
		return m_EnableEventHandler;
	else if (!m_EnableEventHandler.IsEmpty())
		return m_EnableEventHandler;
	else
		return true;
}

void Service::SetEnableEventHandler(bool enabled)
{
	m_OverrideEnableEventHandler = enabled;
}

EventCommand::Ptr Service::GetEventCommand(void) const
{
	return EventCommand::GetByName(m_EventCommand);
}

void Service::ExecuteEventHandler(void)
{
	if (!IcingaApplication::GetInstance()->GetEnableEventHandlers() || !GetEnableEventHandler())
		return;

	EventCommand::Ptr ec = GetEventCommand();

	if (!ec)
		return;

	Log(LogDebug, "icinga", "Executing event handler for service '" + GetName() + "'");

	ec->Execute(GetSelf());

	OnEventCommandExecuted(GetSelf());
}