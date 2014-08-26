/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/checkable.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/logger_fwd.hpp"
#include "base/context.hpp"

using namespace icinga;

boost::signals2::signal<void (const Checkable::Ptr&)> Checkable::OnEventCommandExecuted;
boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> Checkable::OnEnableEventHandlerChanged;

bool Checkable::GetEnableEventHandler(void) const
{
	if (!GetOverrideEnableEventHandler().IsEmpty())
		return GetOverrideEnableEventHandler();
	else
		return GetEnableEventHandlerRaw();
}

void Checkable::SetEnableEventHandler(bool enabled, const MessageOrigin& origin)
{
	SetOverrideEnableEventHandler(enabled);

	OnEnableEventHandlerChanged(GetSelf(), enabled, origin);
}

EventCommand::Ptr Checkable::GetEventCommand(void) const
{
	String command;

	if (!GetOverrideEventCommand().IsEmpty())
		command = GetOverrideEventCommand();
	else
		command = GetEventCommandRaw();

	return EventCommand::GetByName(command);
}

void Checkable::SetEventCommand(const EventCommand::Ptr& command)
{
	SetOverrideEventCommand(command->GetName());
}

void Checkable::ExecuteEventHandler(void)
{
	CONTEXT("Executing event handler for object '" + GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnableEventHandlers() || !GetEnableEventHandler())
		return;

	EventCommand::Ptr ec = GetEventCommand();

	if (!ec)
		return;

	Log(LogNotice, "Checkable", "Executing event handler '" + ec->GetName() + "' for service '" + GetName() + "'");

	ec->Execute(GetSelf());

	OnEventCommandExecuted(GetSelf());
}
