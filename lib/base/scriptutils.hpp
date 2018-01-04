/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef SCRIPTUTILS_H
#define SCRIPTUTILS_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/type.hpp"
#include "base/configobject.hpp"

namespace icinga
{

/**
 * @ingroup base
 */
class ScriptUtils
{
public:
	static void StaticInitialize();
	static String CastString(const Value& value);
	static double CastNumber(const Value& value);
	static bool CastBool(const Value& value);
	static bool Regex(const std::vector<Value>& args);
	static bool Match(const std::vector<Value>& args);
	static bool CidrMatch(const std::vector<Value>& args);
	static double Len(const Value& value);
	static Array::Ptr Union(const std::vector<Value>& arguments);
	static Array::Ptr Intersection(const std::vector<Value>& arguments);
	static void Log(const std::vector<Value>& arguments);
	static Array::Ptr Range(const std::vector<Value>& arguments);
	static Type::Ptr TypeOf(const Value& value);
	static Array::Ptr Keys(const Dictionary::Ptr& dict);
	static ConfigObject::Ptr GetObject(const Value& type, const String& name);
	static Array::Ptr GetObjects(const Type::Ptr& type);
	static void Assert(const Value& arg);
	static String MsiGetComponentPathShim(const String& component);
	static Array::Ptr TrackParents(const Object::Ptr& parent);
	static double Ptr(const Object::Ptr& object);
	static Value Glob(const std::vector<Value>& args);
	static Value GlobRecursive(const std::vector<Value>& args);

private:
	ScriptUtils();
};

}

#endif /* SCRIPTUTILS_H */
