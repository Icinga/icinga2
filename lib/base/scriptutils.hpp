/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
	static Array::Ptr Keys(const Object::Ptr& obj);
	static Dictionary::Ptr GetTemplate(const Value& vtype, const String& name);
	static Array::Ptr GetTemplates(const Type::Ptr& type);
	static ConfigObject::Ptr GetObject(const Value& type, const String& name);
	static Array::Ptr GetObjects(const Type::Ptr& type);
	static void Assert(const Value& arg);
	static String MsiGetComponentPathShim(const String& component);
	static Array::Ptr TrackParents(const Object::Ptr& parent);
	static double Ptr(const Object::Ptr& object);
	static Value Glob(const std::vector<Value>& args);
	static Value GlobRecursive(const std::vector<Value>& args);
	static String GetEnv(const String& key);

private:
	ScriptUtils();
};

}
