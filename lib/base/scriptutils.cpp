// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/scriptutils.hpp"
#include "base/function.hpp"
#include "base/scriptframe.hpp"
#include "base/exception.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/configtype.hpp"
#include "base/application.hpp"
#include "base/initialize.hpp"
#include "base/namespace.hpp"
#include "config/configitem.hpp"
#include <boost/regex.hpp>
#include <algorithm>
#include <set>
#ifdef _WIN32
#include <msi.h>
#endif /* _WIN32 */

using namespace icinga;

REGISTER_SAFE_FUNCTION(System, regex, &ScriptUtils::Regex, "pattern:text:mode");
REGISTER_SAFE_FUNCTION(System, match, &ScriptUtils::Match, "pattern:text:mode");
REGISTER_SAFE_FUNCTION(System, cidr_match, &ScriptUtils::CidrMatch, "pattern:ip:mode");
REGISTER_SAFE_FUNCTION(System, len, &ScriptUtils::Len, "value");
REGISTER_SAFE_FUNCTION(System, union, &ScriptUtils::Union, "");
REGISTER_SAFE_FUNCTION(System, intersection, &ScriptUtils::Intersection, "");
REGISTER_FUNCTION(System, log, &ScriptUtils::Log, "severity:facility:value");
REGISTER_FUNCTION(System, range, &ScriptUtils::Range, "start:end:increment");
REGISTER_FUNCTION(System, exit, &Application::Exit, "status");
REGISTER_SAFE_FUNCTION(System, typeof, &ScriptUtils::TypeOf, "value");
REGISTER_SAFE_FUNCTION(System, keys, &ScriptUtils::Keys, "value");
REGISTER_SAFE_FUNCTION(System, random, &Utility::Random, "");
REGISTER_FUNCTION(System, get_template, &ScriptUtils::GetTemplate, "type:name");
REGISTER_FUNCTION(System, get_templates, &ScriptUtils::GetTemplates, "type");
REGISTER_SAFE_FUNCTION(System, get_object, &ScriptUtils::GetObject, "type:name");
REGISTER_FUNCTION(System, get_objects, &ScriptUtils::GetObjects, "type");
REGISTER_FUNCTION(System, assert, &ScriptUtils::Assert, "value");
REGISTER_SAFE_FUNCTION(System, string, &ScriptUtils::CastString, "value");
REGISTER_SAFE_FUNCTION(System, number, &ScriptUtils::CastNumber, "value");
REGISTER_SAFE_FUNCTION(System, bool, &ScriptUtils::CastBool, "value");
REGISTER_SAFE_FUNCTION(System, get_time, &Utility::GetTime, "");
REGISTER_SAFE_FUNCTION(System, basename, &Utility::BaseName, "path");
REGISTER_SAFE_FUNCTION(System, dirname, &Utility::DirName, "path");
REGISTER_FUNCTION(System, getenv, &ScriptUtils::GetEnv, "value");
REGISTER_SAFE_FUNCTION(System, msi_get_component_path, &ScriptUtils::MsiGetComponentPathShim, "component");
REGISTER_SAFE_FUNCTION(System, escape_shell_cmd, &Utility::EscapeShellCmd, "cmd");
REGISTER_SAFE_FUNCTION(System, escape_shell_arg, &Utility::EscapeShellArg, "arg");
#ifdef _WIN32
REGISTER_SAFE_FUNCTION(System, escape_create_process_arg, &Utility::EscapeCreateProcessArg, "arg");
#endif /* _WIN32 */
REGISTER_FUNCTION(System, ptr, &ScriptUtils::Ptr, "object");
REGISTER_FUNCTION(System, sleep, &Utility::Sleep, "interval");
REGISTER_FUNCTION(System, path_exists, &Utility::PathExists, "path");
REGISTER_FUNCTION(System, glob, &ScriptUtils::Glob, "pathspec:callback:type");
REGISTER_FUNCTION(System, glob_recursive, &ScriptUtils::GlobRecursive, "pathspec:callback:type");

INITIALIZE_ONCE(&ScriptUtils::StaticInitialize);

enum MatchType
{
	MatchAll,
	MatchAny
};

void ScriptUtils::StaticInitialize()
{
	ScriptGlobal::Set("System.MatchAll", MatchAll);
	ScriptGlobal::Set("System.MatchAny", MatchAny);

	ScriptGlobal::Set("System.GlobFile", GlobFile);
	ScriptGlobal::Set("System.GlobDirectory", GlobDirectory);
}

String ScriptUtils::CastString(const Value& value)
{
	return value;
}

double ScriptUtils::CastNumber(const Value& value)
{
	return value;
}

bool ScriptUtils::CastBool(const Value& value)
{
	return value.ToBool();
}

bool ScriptUtils::Regex(const std::vector<Value>& args)
{
	if (args.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Regular expression and text must be specified for regex()."));

	String pattern = args[0];
	const Value& argTexts = args[1];

	if (argTexts.IsObjectType<Dictionary>())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Dictionaries are not supported by regex()."));

	MatchType mode;

	if (args.size() > 2)
		mode = static_cast<MatchType>(static_cast<int>(args[2]));
	else
		mode = MatchAll;

	boost::regex expr(pattern.GetData());

	Array::Ptr texts;

	if (argTexts.IsObject())
		texts = argTexts;

	if (texts) {
		ObjectLock olock(texts);

		if (texts->GetLength() == 0)
			return false;

		for (String text : texts) {
			bool res = false;
			try {
				boost::smatch what;
				res = boost::regex_search(text.GetData(), what, expr);
			} catch (boost::exception&) {
				res = false; /* exception means something went terribly wrong */
			}

			if (mode == MatchAny && res)
				return true;
			else if (mode == MatchAll && !res)
				return false;
		}

		/* MatchAny: Nothing matched. MatchAll: Everything matched. */
		return mode == MatchAll;
	} else {
		String text = argTexts;
		boost::smatch what;
		return boost::regex_search(text.GetData(), what, expr);
	}
}

bool ScriptUtils::Match(const std::vector<Value>& args)
{
	if (args.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Pattern and text must be specified for match()."));

	String pattern = args[0];
	const Value& argTexts = args[1];

	if (argTexts.IsObjectType<Dictionary>())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Dictionaries are not supported by match()."));

	MatchType mode;

	if (args.size() > 2)
		mode = static_cast<MatchType>(static_cast<int>(args[2]));
	else
		mode = MatchAll;

	Array::Ptr texts;

	if (argTexts.IsObject())
		texts = argTexts;

	if (texts) {
		ObjectLock olock(texts);

		if (texts->GetLength() == 0)
			return false;

		for (String text : texts) {
			bool res = Utility::Match(pattern, text);

			if (mode == MatchAny && res)
				return true;
			else if (mode == MatchAll && !res)
				return false;
		}

		/* MatchAny: Nothing matched. MatchAll: Everything matched. */
		return mode == MatchAll;
	} else {
		String text = argTexts;
		return Utility::Match(pattern, argTexts);
	}
}

bool ScriptUtils::CidrMatch(const std::vector<Value>& args)
{
	if (args.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("CIDR and IP address must be specified for cidr_match()."));

	String pattern = args[0];
	const Value& argIps = args[1];

	if (argIps.IsObjectType<Dictionary>())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Dictionaries are not supported by cidr_match()."));

	MatchType mode;

	if (args.size() > 2)
		mode = static_cast<MatchType>(static_cast<int>(args[2]));
	else
		mode = MatchAll;

	Array::Ptr ips;

	if (argIps.IsObject())
		ips = argIps;

	if (ips) {
		ObjectLock olock(ips);

		if (ips->GetLength() == 0)
			return false;

		for (String ip : ips) {
			bool res = Utility::CidrMatch(pattern, ip);

			if (mode == MatchAny && res)
				return true;
			else if (mode == MatchAll && !res)
				return false;
		}

		/* MatchAny: Nothing matched. MatchAll: Everything matched. */
		return mode == MatchAll;
	} else {
		String ip = argIps;
		return Utility::CidrMatch(pattern, ip);
	}
}

double ScriptUtils::Len(const Value& value)
{
	if (value.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = value;
		return dict->GetLength();
	} else if (value.IsObjectType<Array>()) {
		Array::Ptr array = value;
		return array->GetLength();
	} else if (value.IsString()) {
		return Convert::ToString(value).GetLength();
	} else {
		return 0;
	}
}

Array::Ptr ScriptUtils::Union(const std::vector<Value>& arguments)
{
	std::set<Value> values;

	for (const Value& varr : arguments) {
		Array::Ptr arr = varr;

		if (arr) {
			ObjectLock olock(arr);
			for (const Value& value : arr) {
				values.insert(value);
			}
		}
	}

	return Array::FromSet(values);
}

Array::Ptr ScriptUtils::Intersection(const std::vector<Value>& arguments)
{
	if (arguments.size() == 0)
		return new Array();

	Array::Ptr result = new Array();

	Array::Ptr arg1 = arguments[0];

	if (!arg1)
		return result;

	Array::Ptr arr1 = arg1->ShallowClone();

	for (std::vector<Value>::size_type i = 1; i < arguments.size(); i++) {
		{
			ObjectLock olock(arr1);
			std::sort(arr1->Begin(), arr1->End());
		}

		Array::Ptr arg2 = arguments[i];

		if (!arg2)
			return result;

		Array::Ptr arr2 = arg2->ShallowClone();
		{
			ObjectLock olock(arr2);
			std::sort(arr2->Begin(), arr2->End());
		}

		result->Resize(std::max(arr1->GetLength(), arr2->GetLength()));
		Array::SizeType len;
		{
			ObjectLock olock(arr1), xlock(arr2), ylock(result);
			auto it = std::set_intersection(arr1->Begin(), arr1->End(), arr2->Begin(), arr2->End(), result->Begin());
			len = it - result->Begin();
		}
		result->Resize(len);
		arr1 = result;
	}

	return result;
}

void ScriptUtils::Log(const std::vector<Value>& arguments)
{
	if (arguments.size() != 1 && arguments.size() != 3)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid number of arguments for log()"));

	LogSeverity severity;
	String facility;
	Value message;

	if (arguments.size() == 1) {
		severity = LogInformation;
		facility = "config";
		message = arguments[0];
	} else {
		auto sval = static_cast<int>(arguments[0]);
		severity = static_cast<LogSeverity>(sval);
		facility = arguments[1];
		message = arguments[2];
	}

	if (message.IsString() || (!message.IsObjectType<Array>() && !message.IsObjectType<Dictionary>()))
		::Log(severity, facility, message);
	else
		::Log(severity, facility, JsonEncode(message));
}

Array::Ptr ScriptUtils::Range(const std::vector<Value>& arguments)
{
	double start, end, increment;

	switch (arguments.size()) {
		case 1:
			start = 0;
			end = arguments[0];
			increment = 1;
			break;
		case 2:
			start = arguments[0];
			end = arguments[1];
			increment = 1;
			break;
		case 3:
			start = arguments[0];
			end = arguments[1];
			increment = arguments[2];
			break;
		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid number of arguments for range()"));
	}

	ArrayData result;

	if ((start < end && increment <= 0) ||
		(start > end && increment >= 0))
		return new Array();

	for (double i = start; (increment > 0 ? i < end : i > end); i += increment)
		result.push_back(i);

	return new Array(std::move(result));
}

Type::Ptr ScriptUtils::TypeOf(const Value& value)
{
	return value.GetReflectionType();
}

Array::Ptr ScriptUtils::Keys(const Object::Ptr& obj)
{
	ArrayData result;

	Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(obj);

	if (dict) {
		ObjectLock olock(dict);
		for (const Dictionary::Pair& kv : dict) {
			result.push_back(kv.first);
		}
	}

	Namespace::Ptr ns = dynamic_pointer_cast<Namespace>(obj);

	if (ns) {
		ObjectLock olock(ns);
		for (const Namespace::Pair& kv : ns) {
			result.push_back(kv.first);
		}
	}

	return new Array(std::move(result));
}

static Dictionary::Ptr GetTargetForTemplate(const ConfigItem::Ptr& item)
{
	DebugInfo di = item->GetDebugInfo();

	return new Dictionary({
		{ "name", item->GetName() },
		{ "type", item->GetType()->GetName() },
		{ "location", new Dictionary({
			{ "path", di.Path },
			{ "first_line", di.FirstLine },
			{ "first_column", di.FirstColumn },
			{ "last_line", di.LastLine },
			{ "last_column", di.LastColumn }
		}) }
	});
}

Dictionary::Ptr ScriptUtils::GetTemplate(const Value& vtype, const String& name)
{
	Type::Ptr ptype;

	if (vtype.IsObjectType<Type>())
		ptype = vtype;
	else
		ptype = Type::GetByName(vtype);

	ConfigItem::Ptr item = ConfigItem::GetByTypeAndName(ptype, name);

	if (!item || !item->IsAbstract())
		return nullptr;

	DebugInfo di = item->GetDebugInfo();

	return GetTargetForTemplate(item);
}

Array::Ptr ScriptUtils::GetTemplates(const Type::Ptr& type)
{
	if (!type)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type: Must not be null"));

	ArrayData result;

	for (const ConfigItem::Ptr& item : ConfigItem::GetItems(type)) {
		if (item->IsAbstract())
			result.push_back(GetTargetForTemplate(item));
	}

	return new Array(std::move(result));
}

ConfigObject::Ptr ScriptUtils::GetObject(const Value& vtype, const String& name)
{
	Type::Ptr ptype;

	if (vtype.IsObjectType<Type>())
		ptype = vtype;
	else
		ptype = Type::GetByName(vtype);

	auto *ctype = dynamic_cast<ConfigType *>(ptype.get());

	if (!ctype)
		return nullptr;

	auto cfgObj = ctype->GetObject(name);
	if (cfgObj) {
		auto* frame = ScriptFrame::GetCurrentFrame();
		if (frame->PermChecker->CanAccessConfigObject(cfgObj)) {
			return cfgObj;
		}
	}

	return nullptr;
}

Array::Ptr ScriptUtils::GetObjects(const Type::Ptr& type)
{
	if (!type)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type: Must not be null"));

	auto *ctype = dynamic_cast<ConfigType *>(type.get());

	if (!ctype)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type: Type must inherit from 'ConfigObject'"));

	ArrayData result;

	for (const ConfigObject::Ptr& object : ctype->GetObjects())
		result.push_back(object);

	return new Array(std::move(result));
}

void ScriptUtils::Assert(const Value& arg)
{
	if (!arg.ToBool())
		BOOST_THROW_EXCEPTION(std::runtime_error("Assertion failed"));
}

String ScriptUtils::MsiGetComponentPathShim([[maybe_unused]] const String& component)
{
#ifdef _WIN32
	TCHAR productCode[39];
	if (MsiGetProductCode(component.CStr(), productCode) != ERROR_SUCCESS)
		return "";
	TCHAR path[2048];
	DWORD szPath = sizeof(path);
	path[0] = '\0';
	MsiGetComponentPath(productCode, component.CStr(), path, &szPath);
	return path;
#else /* _WIN32 */
	return String();
#endif /* _WIN32 */
}

double ScriptUtils::Ptr(const Object::Ptr& object)
{
	return reinterpret_cast<intptr_t>(object.get());
}

Value ScriptUtils::Glob(const std::vector<Value>& args)
{
	if (args.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Path must be specified."));

	String pathSpec = args[0];
	int type = GlobFile | GlobDirectory;

	if (args.size() > 1)
		type = args[1];

	std::vector<String> paths;
	Utility::Glob(pathSpec, [&paths](const String& path) { paths.push_back(path); }, type);

	return Array::FromVector(paths);
}

Value ScriptUtils::GlobRecursive(const std::vector<Value>& args)
{
	if (args.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Path and pattern must be specified."));

	String path = args[0];
	String pattern = args[1];

	int type = GlobFile | GlobDirectory;

	if (args.size() > 2)
		type = args[2];

	std::vector<String> paths;
	Utility::GlobRecursive(path, pattern, [&paths](const String& newPath) { paths.push_back(newPath); }, type);

	return Array::FromVector(paths);
}

String ScriptUtils::GetEnv(const String& key)
{
	return Utility::GetFromEnvironment(key);
}
