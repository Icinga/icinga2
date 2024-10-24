/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
	CONTEXT("Executing event handler for object '" << GetName() << "'");

	if (!IcingaApplication::GetInstance()->GetEnableEventHandlers() || !GetEnableEventHandler())
		return;

	auto zone (GetZone());

	if (zone && zone != Zone::GetLocalZone()) {
		Log(LogNotice, "Checkable")
			<< "Skipping event handler for checkable '" << GetName()
			<< "' in child zone '" << zone->GetName() << "'";
		return;
	}

	/* HA enabled zones. */
	if (IsActive() && IsPaused()) {
		Log(LogNotice, "Checkable")
			<< "Skipping event handler for HA-paused checkable '" << GetName() << "'";
		return;
	}

	EventCommand::Ptr ec = GetEventCommand();

	if (!ec)
		return;

	Log(LogNotice, "Checkable")
		<< "Executing event handler '" << ec->GetName() << "' for checkable '" << GetName() << "'";

	Dictionary::Ptr macros;
	Endpoint::Ptr endpoint = GetCommandEndpoint();

	if (endpoint && !useResolvedMacros)
		macros = new Dictionary();
	else
		macros = resolvedMacros;

	ec->Execute(this, macros, useResolvedMacros);

	if (endpoint && !GetExtension("agent_check")) {
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
