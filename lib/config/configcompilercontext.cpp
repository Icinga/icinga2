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

#include "config/configcompilercontext.h"
#include "base/utility.h"
#include "base/logger_fwd.h"
#include "base/singleton.h"
#include <boost/foreach.hpp>

using namespace icinga;

void ConfigCompilerContext::AddMessage(bool error, const String& message)
{
	m_Messages.push_back(ConfigCompilerMessage(error, message));
}

std::vector<ConfigCompilerMessage> ConfigCompilerContext::GetMessages(void) const
{
	return m_Messages;
}

bool ConfigCompilerContext::HasErrors(void) const
{
	BOOST_FOREACH(const ConfigCompilerMessage& message, m_Messages) {
		if (message.Error)
			return true;
	}

	return false;
}

void ConfigCompilerContext::Reset(void)
{
	m_Messages.clear();
}

ConfigCompilerContext *ConfigCompilerContext::GetInstance(void)
{
	return Singleton<ConfigCompilerContext>::GetInstance();
}