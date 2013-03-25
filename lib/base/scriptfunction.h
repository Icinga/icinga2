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

#ifndef SCRIPTFUNCTION_H
#define SCRIPTFUNCTION_H

#include "base/i2-base.h"
#include "base/registry.h"
#include "base/value.h"
#include <vector>
#include <boost/function.hpp>

namespace icinga
{

class ScriptTask;

/**
 * A script function that can be used to execute a script task.
 *
 * @ingroup base
 */
class I2_BASE_API ScriptFunction : public Object
{
public:
	typedef shared_ptr<ScriptFunction> Ptr;
	typedef weak_ptr<ScriptFunction> WeakPtr;

	typedef boost::function<Value (const std::vector<Value>& arguments)> Callback;

	explicit ScriptFunction(const Callback& function);

	Value Invoke(const std::vector<Value>& arguments);

private:
	Callback m_Callback;
};

/**
 * A registry for script functions.
 *
 * @ingroup base
 */
class I2_BASE_API ScriptFunctionRegistry : public Registry<ScriptFunction::Ptr>
{ };

/**
 * Helper class for registering ScriptFunction implementation classes.
 *
 * @ingroup base
 */
class I2_BASE_API RegisterFunctionHelper
{
public:
	RegisterFunctionHelper(const String& name, const ScriptFunction::Callback& function);
};

#define REGISTER_SCRIPTFUNCTION(name, callback) \
	I2_EXPORT icinga::RegisterFunctionHelper g_RegisterSF_ ## name(#name, callback)

}

#endif /* SCRIPTFUNCTION_H */
