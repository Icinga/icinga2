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

#ifndef CONFIGCOMPILERCONTEXT_H
#define CONFIGCOMPILERCONTEXT_H

#include "config/i2-config.h"
#include "config/configitem.h"
#include "config/configtype.h"

namespace icinga
{

struct I2_CONFIG_API ConfigCompilerError
{
	bool Warning;
	String Message;

	ConfigCompilerError(bool warning, const String& message)
		: Warning(warning), Message(message)
	{ }
};

/*
 * @ingroup config
 */
class I2_CONFIG_API ConfigCompilerContext
{
public:
	void AddError(bool warning, const String& message);
	std::vector<ConfigCompilerError> GetErrors(void) const;

	void Reset(void);

	static ConfigCompilerContext *GetInstance(void);

private:
        std::vector<ConfigCompilerError> m_Errors;
};

}

#endif /* CONFIGCOMPILERCONTEXT_H */
