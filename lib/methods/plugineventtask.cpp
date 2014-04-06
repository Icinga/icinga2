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

#include "methods/plugineventtask.h"
#include "icinga/eventcommand.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/scriptfunction.h"
#include "base/utility.h"
#include "base/process.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(PluginEvent, &PluginEventTask::ScriptFunc);

void PluginEventTask::ScriptFunc(const Checkable::Ptr& checkable)
{
	EventCommand::Ptr commandObj = checkable->GetEventCommand();
	Value raw_command = commandObj->GetCommandLine();

	bool is_service = checkable->GetType() == DynamicType::GetByName("Service");
	Host::Ptr host;
	Service::Ptr service;

	if (is_service) {
		service = static_pointer_cast<Service>(checkable);
		host = service->GetHost();
	} else
		host = static_pointer_cast<Host>(checkable);

	std::vector<MacroResolver::Ptr> resolvers;
	if (is_service)
		resolvers.push_back(service);
	resolvers.push_back(host);
	resolvers.push_back(commandObj);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value command = MacroProcessor::ResolveMacros(raw_command, resolvers, checkable->GetLastCheckResult(), Utility::EscapeShellCmd);

	Dictionary::Ptr envMacros = make_shared<Dictionary>();

	Dictionary::Ptr env = commandObj->GetEnv();

	if (env) {
		BOOST_FOREACH(const Dictionary::Pair& kv, env) {
			String name = kv.second;

			Value value = MacroProcessor::ResolveMacros(name, resolvers, checkable->GetLastCheckResult());

			envMacros->Set(kv.first, value);
		}
	}

	Process::Ptr process = make_shared<Process>(Process::SplitCommand(command), envMacros);

	process->SetTimeout(commandObj->GetTimeout());

	process->Run();
}
