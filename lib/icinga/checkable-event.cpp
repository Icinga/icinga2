/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "icinga/service.hpp"
#include "remote/apilistener.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"

using namespace icinga;

boost::signals2::signal<void (const Checkable::Ptr&)> Checkable::OnEventCommandExecuted;

EventCommand::Ptr Checkable::GetEventCommand() const
{
	return EventCommand::GetByName(GetEventCommandRaw());
}

void Checkable::ExecuteEventHandler(const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	CONTEXT("Executing event handler for object '" + GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnableEventHandlers() || !GetEnableEventHandler())
		return;

	EventCommand::Ptr ec = GetEventCommand();

	if (!ec)
		return;

	Log(LogNotice, "Checkable")
		<< "Executing event handler '" << ec->GetName() << "' for service '" << GetName() << "'";

	Dictionary::Ptr macros;
	Endpoint::Ptr endpoint = GetCommandEndpoint();

	if (endpoint && !useResolvedMacros)
		macros = new Dictionary();
	else
		macros = resolvedMacros;

	ec->Execute(this, macros, useResolvedMacros);

	if (endpoint) {
		Dictionary::Ptr message = new Dictionary();
		message->Set("jsonrpc", "2.0");
		message->Set("method", "event::ExecuteCommand");

		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(this);

		Dictionary::Ptr params = new Dictionary();
		message->Set("params", params);
		params->Set("command_type", "event_command");
		params->Set("command", GetEventCommand()->GetName());
		params->Set("host", host->GetName());

		if (service)
			params->Set("service", service->GetShortName());

		params->Set("macros", macros);

		ApiListener::Ptr listener = ApiListener::GetInstance();

		if (listener)
			listener->SyncSendMessage(endpoint, message);

		return;
	}

	OnEventCommandExecuted(this);
}
