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

#ifndef SCRIPTVARIABLE_H
#define SCRIPTVARIABLE_H

#include "base/i2-base.h"
#include "base/registry.h"
#include "base/singleton.h"
#include "base/value.h"

namespace icinga
{

/**
 * A script variables.
 *
 * @ingroup base
 */
class I2_BASE_API ScriptVariable : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ScriptVariable);

	ScriptVariable(const Value& data);

	void SetConstant(bool constant);
	bool IsConstant(void) const;

	void SetData(const Value& data);
	Value GetData(void) const;

	static ScriptVariable::Ptr GetByName(const String& name);

	static Value Get(const String& name);
	static ScriptVariable::Ptr Set(const String& name, const Value& value, bool overwrite = true, bool make_const = false);

private:
	Value m_Data;
	bool m_Constant;
};

class I2_BASE_API ScriptVariableRegistry : public Registry<ScriptVariableRegistry, ScriptVariable::Ptr>
{
public:
	static ScriptVariableRegistry *GetInstance(void);
};

}

#endif /* SCRIPTVARIABLE_H */
