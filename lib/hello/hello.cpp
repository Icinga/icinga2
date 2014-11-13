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

#include "hello/hello.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "base/dynamictype.hpp"
#include "base/logger.hpp"
#include "base/json.hpp"
#include "base/serializer.hpp"
#include <iostream>

using namespace icinga;

REGISTER_TYPE(Hello);

/**
 * The entry point for the Hello application.
 *
 * @returns An exit status.
 */
int Hello::Main(void)
{
	Log(LogInformation, "Hello", "Hello World!");

	Host::Ptr host = Host::GetByName("test");
	CheckCommand::Ptr command = host->GetCheckCommand();

	Dictionary::Ptr macros = new Dictionary();

	command->Execute(host, CheckResult::Ptr(), macros);

	std::cout << JsonEncode(macros) << std::endl;

	Host::Ptr host2 = new Host();
	Dictionary::Ptr attrs = new Dictionary();

	attrs->Set("__name", "keks");
	attrs->Set("type", "Host");
	attrs->Set("check_command", "http");
	attrs->Set("command_endpoint", "test");

	Deserialize(host2, attrs, false, FAConfig);

	host2->SetExtension("agent_service_name", "foobar");

	static_pointer_cast<DynamicObject>(host2)->OnStateLoaded();
	static_pointer_cast<DynamicObject>(host2)->OnConfigLoaded();

	std::cout << host2->GetName() << std::endl;

	host2->ExecuteCheck(macros, true);

	Utility::Sleep(30);

	return 0;
}
