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

#include "base/scriptinterpreter.h"
#include "base/scriptfunction.h"
#include "base/objectlock.h"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

ScriptInterpreter::ScriptInterpreter(const Script::Ptr&)
{ }

ScriptInterpreter::~ScriptInterpreter(void)
{
	BOOST_FOREACH(const String& function, m_SubscribedFunctions) {
		ScriptFunctionRegistry::GetInstance()->Unregister(function);
	}
}

void ScriptInterpreter::SubscribeFunction(const String& name)
{
	ObjectLock olock(this);

	m_SubscribedFunctions.insert(name);

	ScriptFunction::Ptr sf = boost::make_shared<ScriptFunction>(boost::bind(&ScriptInterpreter::ProcessCall, this, name, _1));
	ScriptFunctionRegistry::GetInstance()->Register(name, sf);
}

void ScriptInterpreter::UnsubscribeFunction(const String& name)
{
	ObjectLock olock(this);

	m_SubscribedFunctions.erase(name);
	ScriptFunctionRegistry::GetInstance()->Unregister(name);
}
