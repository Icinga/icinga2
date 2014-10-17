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

#ifndef AGENTUTILITY_H
#define AGENTUTILITY_H

#include "base/i2-base.hpp"
#include "base/value.hpp"

namespace icinga
{

/**
 * @ingroup cli
 */
class AgentUtility
{
public:
	static void ListAgents(void);
	static bool AddAgent(const String& name);
	static bool RemoveAgent(const String& name);
	static bool SetAgentAttribute(const String& attr, const Value& val);

private:
	AgentUtility(void);
};

}

#endif /* AGENTUTILITY_H */
