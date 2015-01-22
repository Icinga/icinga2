/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/i2-base.hpp"
#include "base/registry.hpp"
#include "base/value.hpp"

namespace icinga
{

class ScriptVariable;

class I2_BASE_API ScriptVariableRegistry : public Registry<ScriptVariableRegistry, intrusive_ptr<ScriptVariable> >
{
public:
	static ScriptVariableRegistry *GetInstance(void);
};
	
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
	static void Unregister(const String& name);

	static Value Get(const String& name, const Value *defaultValue = NULL);
	static ScriptVariable::Ptr Set(const String& name, const Value& value, bool overwrite = true, bool make_const = false);

	static void WriteVariablesFile(const String& filename);

private:
	Value m_Data;
	bool m_Constant;
};

}

#endif /* SCRIPTVARIABLE_H */
