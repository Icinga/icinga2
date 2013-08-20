/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

void ConfigCompilerContext::AddError(bool warning, const String& message)
{
	m_Errors.push_back(ConfigCompilerError(warning, message));
}

std::vector<ConfigCompilerError> ConfigCompilerContext::GetErrors(void) const
{
	return m_Errors;
}

void ConfigCompilerContext::Reset(void)
{
	m_Errors.clear();
}

ConfigCompilerContext *ConfigCompilerContext::GetInstance(void)
{
	return Singleton<ConfigCompilerContext>::GetInstance();
}