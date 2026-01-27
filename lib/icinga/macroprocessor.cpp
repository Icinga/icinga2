// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/macroprocessor.hpp"
#include "icinga/macroresolver.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/envresolver.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/configobject.hpp"
#include "base/scriptframe.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

thread_local Dictionary::Ptr MacroResolver::OverrideMacros;

Value MacroProcessor::ResolveMacros(const Value& str, const ResolverList& resolvers,
	const CheckResult::Ptr& cr, String *missingMacro,
	const MacroProcessor::EscapeCallback& escapeFn, const Dictionary::Ptr& resolvedMacros,
	bool useResolvedMacros, int recursionLevel)
{
	if (useResolvedMacros)
		REQUIRE_NOT_NULL(resolvedMacros);

	Value result;

	if (str.IsEmpty())
		return Empty;

	if (str.IsScalar()) {
		result = InternalResolveMacros(str, resolvers, cr, missingMacro, escapeFn,
			resolvedMacros, useResolvedMacros, recursionLevel + 1);
	} else if (str.IsObjectType<Array>()) {
		ArrayData resultArr;
		Array::Ptr arr = str;

		ObjectLock olock(arr);

		for (const Value& arg : arr) {
			/* Note: don't escape macros here. */
			Value value = InternalResolveMacros(arg, resolvers, cr, missingMacro,
				EscapeCallback(), resolvedMacros, useResolvedMacros, recursionLevel + 1);

			if (value.IsObjectType<Array>())
				resultArr.push_back(Utility::Join(value, ';'));
			else
				resultArr.push_back(value);
		}

		result = new Array(std::move(resultArr));
	} else if (str.IsObjectType<Dictionary>()) {
		Dictionary::Ptr resultDict = new Dictionary();
		Dictionary::Ptr dict = str;

		ObjectLock olock(dict);

		for (const Dictionary::Pair& kv : dict) {
			/* Note: don't escape macros here. */
			resultDict->Set(kv.first, InternalResolveMacros(kv.second, resolvers, cr, missingMacro,
				EscapeCallback(), resolvedMacros, useResolvedMacros, recursionLevel + 1));
		}

		result = resultDict;
	} else if (str.IsObjectType<Function>()) {
		result = EvaluateFunction(str, resolvers, cr, resolvedMacros, useResolvedMacros, 0);
	} else {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Macro is not a string or array."));
	}

	return result;
}

static const EnvResolver::Ptr l_EnvResolver = new EnvResolver();

static MacroProcessor::ResolverList GetDefaultResolvers()
{
	return {
		{ "icinga", IcingaApplication::GetInstance() },
		{ "env", l_EnvResolver, false }
	};
}

bool MacroProcessor::ResolveMacro(const String& macro, const ResolverList& resolvers,
	const CheckResult::Ptr& cr, Value *result, bool *recursive_macro)
{
	CONTEXT("Resolving macro '" << macro << "'");

	*recursive_macro = false;

	std::vector<String> tokens = macro.Split(".");

	String objName;
	if (tokens.size() > 1) {
		objName = tokens[0];
		tokens.erase(tokens.begin());
	}

	const auto defaultResolvers (GetDefaultResolvers());

	for (auto resolverList : {&resolvers, &defaultResolvers}) {
		for (auto& resolver : *resolverList) {
			if (!objName.IsEmpty() && objName != resolver.Name)
				continue;

			if (objName.IsEmpty()) {
				if (!resolver.ResolveShortMacros)
					continue;

				Dictionary::Ptr vars;
				CustomVarObject::Ptr dobj = dynamic_pointer_cast<CustomVarObject>(resolver.Obj);

				if (dobj) {
					vars = dobj->GetVars();
				} else {
					auto app (dynamic_pointer_cast<IcingaApplication>(resolver.Obj));

					if (app) {
						vars = app->GetVars();
					}
				}

				if (vars && vars->Contains(macro)) {
					*result = vars->Get(macro);
					*recursive_macro = true;
					return true;
				}
			}

			auto *mresolver = dynamic_cast<MacroResolver *>(resolver.Obj.get());

			if (mresolver && mresolver->ResolveMacro(boost::algorithm::join(tokens, "."), cr, result))
				return true;

			Value ref = resolver.Obj;
			bool valid = true;

			for (const String& token : tokens) {
				if (ref.IsObjectType<Dictionary>()) {
					Dictionary::Ptr dict = ref;
					if (dict->Contains(token)) {
						ref = dict->Get(token);
						continue;
					} else {
						valid = false;
						break;
					}
				} else if (ref.IsObject()) {
					Object::Ptr object = ref;

					Type::Ptr type = object->GetReflectionType();

					if (!type) {
						valid = false;
						break;
					}

					int field = type->GetFieldId(token);

					if (field == -1) {
						valid = false;
						break;
					}

					ref = object->GetField(field);

					Field fieldInfo = type->GetFieldInfo(field);

					if (strcmp(fieldInfo.TypeName, "Timestamp") == 0)
						ref = static_cast<long>(ref);
				}
			}

			if (valid) {
				if (tokens[0] == "vars" ||
					tokens[0] == "action_url" ||
					tokens[0] == "notes_url" ||
					tokens[0] == "notes")
					*recursive_macro = true;

				*result = ref;
				return true;
			}
		}
	}

	return false;
}

Value MacroProcessor::EvaluateFunction(const Function::Ptr& func, const ResolverList& resolvers,
	const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros, int recursionLevel)
{
	Dictionary::Ptr resolvers_this = new Dictionary();
	const auto defaultResolvers (GetDefaultResolvers());

	for (auto resolverList : {&resolvers, &defaultResolvers}) {
		for (auto& resolver: *resolverList) {
			resolvers_this->Set(resolver.Name, resolver.Obj);
		}
	}

	auto internalResolveMacrosShim = [resolvers, cr, resolvedMacros, useResolvedMacros, recursionLevel](const std::vector<Value>& args) {
		if (args.size() < 1)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function"));

		String missingMacro;

		return MacroProcessor::InternalResolveMacros(args[0], resolvers, cr, &missingMacro, MacroProcessor::EscapeCallback(),
			resolvedMacros, useResolvedMacros, recursionLevel);
	};

	resolvers_this->Set("macro", new Function("macro (temporary)", internalResolveMacrosShim, { "str" }));

	auto internalResolveArgumentsShim = [resolvers, cr, resolvedMacros, useResolvedMacros, recursionLevel](const std::vector<Value>& args) {
		if (args.size() < 2)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function"));

		return MacroProcessor::ResolveArguments(args[0], args[1], resolvers, cr,
			resolvedMacros, useResolvedMacros, recursionLevel + 1);
	};

	resolvers_this->Set("resolve_arguments", new Function("resolve_arguments (temporary)", internalResolveArgumentsShim, { "command", "args" }));

	return func->InvokeThis(resolvers_this);
}

Value MacroProcessor::InternalResolveMacros(const String& str, const ResolverList& resolvers,
	const CheckResult::Ptr& cr, String *missingMacro,
	const MacroProcessor::EscapeCallback& escapeFn, const Dictionary::Ptr& resolvedMacros,
	bool useResolvedMacros, int recursionLevel)
{
	CONTEXT("Resolving macros for string '" << str << "'");

	if (recursionLevel > 15)
		BOOST_THROW_EXCEPTION(std::runtime_error("Infinite recursion detected while resolving macros"));

	size_t offset, pos_first, pos_second;
	offset = 0;

	Dictionary::Ptr resolvers_this;

	String result = str;
	while ((pos_first = result.FindFirstOf("$", offset)) != String::NPos) {
		pos_second = result.FindFirstOf("$", pos_first + 1);

		if (pos_second == String::NPos)
			BOOST_THROW_EXCEPTION(std::runtime_error("Closing $ not found in macro format string."));

		String name = result.SubStr(pos_first + 1, pos_second - pos_first - 1);

		Value resolved_macro;
		bool recursive_macro;
		bool found;

		if (useResolvedMacros) {
			recursive_macro = false;
			found = resolvedMacros->Contains(name);

			if (found)
				resolved_macro = resolvedMacros->Get(name);
		} else
			found = ResolveMacro(name, resolvers, cr, &resolved_macro, &recursive_macro);

		/* $$ is an escape sequence for $. */
		if (name.IsEmpty()) {
			resolved_macro = "$";
			found = true;
		}

		if (resolved_macro.IsObjectType<Function>()) {
			resolved_macro = EvaluateFunction(resolved_macro, resolvers, cr,
				resolvedMacros, useResolvedMacros, recursionLevel + 1);
		}

		if (!found) {
			if (!missingMacro)
				Log(LogWarning, "MacroProcessor")
					<< "Macro '" << name << "' is not defined.";
			else
				*missingMacro = name;
		}

		/* recursively resolve macros in the macro if it was a user macro */
		if (recursive_macro) {
			if (resolved_macro.IsObjectType<Array>()) {
				Array::Ptr arr = resolved_macro;
				ArrayData resolved_arr;

				ObjectLock olock(arr);
				for (const Value& value : arr) {
					if (value.IsScalar()) {
						resolved_arr.push_back(InternalResolveMacros(value,
							resolvers, cr, missingMacro, EscapeCallback(), nullptr,
							false, recursionLevel + 1));
					} else
						resolved_arr.push_back(value);
				}

				resolved_macro = new Array(std::move(resolved_arr));
			} else if (resolved_macro.IsString()) {
				resolved_macro = InternalResolveMacros(resolved_macro,
					resolvers, cr, missingMacro, EscapeCallback(), nullptr,
					false, recursionLevel + 1);
			}
		}

		if (!useResolvedMacros && found && resolvedMacros)
			resolvedMacros->Set(name, resolved_macro);

		if (escapeFn)
			resolved_macro = escapeFn(resolved_macro);

		/* we're done if this is the only macro and there are no other non-macro parts in the string */
		if (pos_first == 0 && pos_second == str.GetLength() - 1)
			return resolved_macro;
		else if (resolved_macro.IsObjectType<Array>())
				BOOST_THROW_EXCEPTION(std::invalid_argument("Mixing both strings and non-strings in macros is not allowed."));

		if (resolved_macro.IsObjectType<Array>()) {
			/* don't allow mixing strings and arrays in macro strings */
			if (pos_first != 0 || pos_second != str.GetLength() - 1)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Mixing both strings and non-strings in macros is not allowed."));

			return resolved_macro;
		}

		String resolved_macro_str = resolved_macro;

		result.Replace(pos_first, pos_second - pos_first + 1, resolved_macro_str);
		offset = pos_first + resolved_macro_str.GetLength();
	}

	return result;
}


bool MacroProcessor::ValidateMacroString(const String& macro)
{
	if (macro.IsEmpty())
		return true;

	size_t pos_first, pos_second, offset;
	offset = 0;

	while ((pos_first = macro.FindFirstOf("$", offset)) != String::NPos) {
		pos_second = macro.FindFirstOf("$", pos_first + 1);

		if (pos_second == String::NPos)
			return false;

		offset = pos_second + 1;
	}

	return true;
}

void MacroProcessor::ValidateCustomVars(const ConfigObject::Ptr& object, const Dictionary::Ptr& value)
{
	if (!value)
		return;

	/* string, array, dictionary */
	ObjectLock olock(value);
	for (const Dictionary::Pair& kv : value) {
		const Value& varval = kv.second;

		if (varval.IsObjectType<Dictionary>()) {
			/* only one dictonary level */
			Dictionary::Ptr varval_dict = varval;

			ObjectLock xlock(varval_dict);
			for (const Dictionary::Pair& kv_var : varval_dict) {
				if (!kv_var.second.IsString())
					continue;

				if (!ValidateMacroString(kv_var.second))
					BOOST_THROW_EXCEPTION(ValidationError(object.get(), { "vars", kv.first, kv_var.first }, "Closing $ not found in macro format string '" + kv_var.second + "'."));
			}
		} else if (varval.IsObjectType<Array>()) {
			/* check all array entries */
			Array::Ptr varval_arr = varval;

			ObjectLock ylock (varval_arr);
			for (const Value& arrval : varval_arr) {
				if (!arrval.IsString())
					continue;

				if (!ValidateMacroString(arrval)) {
					BOOST_THROW_EXCEPTION(ValidationError(object.get(), { "vars", kv.first }, "Closing $ not found in macro format string '" + arrval + "'."));
				}
			}
		} else {
			if (!varval.IsString())
				continue;

			if (!ValidateMacroString(varval))
				BOOST_THROW_EXCEPTION(ValidationError(object.get(), { "vars", kv.first }, "Closing $ not found in macro format string '" + varval + "'."));
		}
	}
}

void MacroProcessor::AddArgumentHelper(const Array::Ptr& args, const String& key, const String& value,
	bool add_key, bool add_value, const Value& separator)
{
	if (add_key && separator.GetType() != ValueEmpty && add_value) {
		args->Add(key + separator + value);
	} else {
		if (add_key)
			args->Add(key);

		if (add_value)
			args->Add(value);
	}
}

Value MacroProcessor::EscapeMacroShellArg(const Value& value)
{
	String result;

	if (value.IsObjectType<Array>()) {
		Array::Ptr arr = value;

		ObjectLock olock(arr);
		for (const Value& arg : arr) {
			if (result.GetLength() > 0)
				result += " ";

			result += Utility::EscapeShellArg(arg);
		}
	} else
		result = Utility::EscapeShellArg(value);

	return result;
}

struct CommandArgument
{
	int Order{0};
	bool SkipKey{false};
	bool RepeatKey{true};
	bool SkipValue{false};
	String Key;
	Value Separator;
	Value AValue;

	bool operator<(const CommandArgument& rhs) const
	{
		return Order < rhs.Order;
	}
};

Value MacroProcessor::ResolveArguments(const Value& command, const Dictionary::Ptr& arguments,
	const MacroProcessor::ResolverList& resolvers, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros, int recursionLevel)
{
	if (useResolvedMacros)
		REQUIRE_NOT_NULL(resolvedMacros);

	Value resolvedCommand;
	if (!arguments || command.IsObjectType<Array>() || command.IsObjectType<Function>())
		resolvedCommand = MacroProcessor::ResolveMacros(command, resolvers, cr, nullptr,
			EscapeMacroShellArg, resolvedMacros, useResolvedMacros, recursionLevel + 1);
	else {
		resolvedCommand = new Array({ command });
	}

	if (arguments) {
		std::vector<CommandArgument> args;

		ObjectLock olock(arguments);
		for (const Dictionary::Pair& kv : arguments) {
			const Value& arginfo = kv.second;

			CommandArgument arg;
			arg.Key = kv.first;

			bool required = false;
			Value argval;

			if (arginfo.IsObjectType<Dictionary>()) {
				Dictionary::Ptr argdict = arginfo;
				if (argdict->Contains("key"))
					arg.Key = argdict->Get("key");
				argval = argdict->Get("value");
				if (argdict->Contains("required"))
					required = argdict->Get("required");
				arg.SkipKey = argdict->Get("skip_key");
				if (argdict->Contains("repeat_key"))
					arg.RepeatKey = argdict->Get("repeat_key");
				arg.Order = argdict->Get("order");
				arg.Separator = argdict->Get("separator");

				Value set_if = argdict->Get("set_if");

				if (!set_if.IsEmpty()) {
					String missingMacro;
					Value set_if_resolved = MacroProcessor::ResolveMacros(set_if, resolvers,
						cr, &missingMacro, MacroProcessor::EscapeCallback(), resolvedMacros,
						useResolvedMacros, recursionLevel + 1);

					if (!missingMacro.IsEmpty())
						continue;

					int value;

					if (set_if_resolved == "true")
						value = 1;
					else if (set_if_resolved == "false")
						value = 0;
					else {
						try {
							value = Convert::ToLong(set_if_resolved);
						} catch (const std::exception& ex) {
							/* tried to convert a string */
							Log(LogWarning, "PluginUtility")
								<< "Error evaluating set_if value '" << set_if_resolved
								<< "' used in argument '" << arg.Key << "': " << ex.what();
							continue;
						}
					}

					if (!value)
						continue;
				}
			}
			else
				argval = arginfo;

			if (argval.IsEmpty())
				arg.SkipValue = true;

			String missingMacro;
			arg.AValue = MacroProcessor::ResolveMacros(argval, resolvers,
				cr, &missingMacro, MacroProcessor::EscapeCallback(), resolvedMacros,
				useResolvedMacros, recursionLevel + 1);

			if (!missingMacro.IsEmpty()) {
				if (required) {
					BOOST_THROW_EXCEPTION(ScriptError("Non-optional macro '" + missingMacro + "' used in argument '" +
						arg.Key + "' is missing."));
				}

				continue;
			}

			args.emplace_back(std::move(arg));
		}

		std::sort(args.begin(), args.end());

		Array::Ptr command_arr = resolvedCommand;
		for (const CommandArgument& arg : args) {

			if (arg.AValue.IsObjectType<Dictionary>()) {
				Log(LogWarning, "PluginUtility")
					<< "Tried to use dictionary in argument '" << arg.Key << "'.";
				continue;
			} else if (arg.AValue.IsObjectType<Array>()) {
				bool first = true;
				Array::Ptr arr = static_cast<Array::Ptr>(arg.AValue);

				ObjectLock olock(arr);
				for (const Value& value : arr) {
					bool add_key;

					if (first) {
						first = false;
						add_key = !arg.SkipKey;
					} else
						add_key = !arg.SkipKey && arg.RepeatKey;

					AddArgumentHelper(command_arr, arg.Key, value, add_key, !arg.SkipValue, arg.Separator);
				}
			} else
				AddArgumentHelper(command_arr, arg.Key, arg.AValue, !arg.SkipKey, !arg.SkipValue, arg.Separator);
		}
	}

	return resolvedCommand;
}
