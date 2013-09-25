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

#include "icinga/plugineventtask.h"
#include "icinga/eventcommand.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/scriptfunction.h"
#include "base/utility.h"
#include "base/process.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(PluginEvent, &PluginEventTask::ScriptFunc);

void PluginEventTask::ScriptFunc(const Service::Ptr& service)
{
	EventCommand::Ptr commandObj = service->GetEventCommand();
	Value raw_command = commandObj->GetCommandLine();

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(service);
	resolvers.push_back(service->GetHost());
	resolvers.push_back(commandObj);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value command = MacroProcessor::ResolveMacros(raw_command, resolvers, Dictionary::Ptr(), Utility::EscapeShellCmd, commandObj->GetEscapeMacros());

	Dictionary::Ptr envMacros = boost::make_shared<Dictionary>();

	Array::Ptr export_macros = commandObj->GetExportMacros();

	if (export_macros) {
		BOOST_FOREACH(const String& macro, export_macros) {
			String value;

			if (!MacroProcessor::ResolveMacro(macro, resolvers, Dictionary::Ptr(), &value)) {
				Log(LogWarning, "icinga", "export_macros for command '" + commandObj->GetName() + "' refers to unknown macro '" + macro + "'");
				continue;
			}

			envMacros->Set(macro, value);
		}
	}

	Process::Ptr process = boost::make_shared<Process>(Process::SplitCommand(command), envMacros);

	process->SetTimeout(commandObj->GetTimeout());

	process->Run();
}
