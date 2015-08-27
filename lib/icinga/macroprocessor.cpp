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

#include "icinga/macroprocessor.hpp"
#include "icinga/macroresolver.hpp"
#include "icinga/customvarobject.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/dynamicobject.hpp"
#include "base/scriptframe.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

Value MacroProcessor::ResolveMacros(const Value& str, const ResolverList& resolvers,
    const CheckResult::Ptr& cr, String *missingMacro,
    const MacroProcessor::EscapeCallback& escapeFn, const Dictionary::Ptr& resolvedMacros,
    bool useResolvedMacros)
{
	Value result;

	if (str.IsEmpty())
		return Empty;

	if (str.IsScalar()) {
		result = InternalResolveMacros(str, resolvers, cr, missingMacro, escapeFn,
		    resolvedMacros, useResolvedMacros);
	} else if (str.IsObjectType<Array>()) {
		Array::Ptr resultArr = new Array();
		Array::Ptr arr = str;

		ObjectLock olock(arr);

		BOOST_FOREACH(const Value& arg, arr) {
			/* Note: don't escape macros here. */
			Value value = InternalResolveMacros(arg, resolvers, cr, missingMacro,
			    EscapeCallback(), resolvedMacros, useResolvedMacros);

			if (value.IsObjectType<Array>())
				resultArr->Add(Utility::Join(value, ';'));
			else
				resultArr->Add(value);
		}

		result = resultArr;
	} else if (str.IsObjectType<Dictionary>()) {
		Dictionary::Ptr resultDict = new Dictionary();
		Dictionary::Ptr dict = str;

		ObjectLock olock(dict);

		BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
			/* Note: don't escape macros here. */
			resultDict->Set(kv.first, InternalResolveMacros(kv.second, resolvers, cr, missingMacro,
			    EscapeCallback(), resolvedMacros, useResolvedMacros));
		}

		result = resultDict;
	} else if (str.IsObjectType<Function>()) {
		result = EvaluateFunction(str, resolvers, cr, escapeFn, resolvedMacros, useResolvedMacros, 0);
	} else {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Macro is not a string or array."));
	}

	return result;
}

bool MacroProcessor::ResolveMacro(const String& macro, const ResolverList& resolvers,
    const CheckResult::Ptr& cr, Value *result, bool *recursive_macro)
{
	CONTEXT("Resolving macro '" + macro + "'");

	*recursive_macro = false;

	std::vector<String> tokens;
	boost::algorithm::split(tokens, macro, boost::is_any_of("."));

	String objName;
	if (tokens.size() > 1) {
		objName = tokens[0];
		tokens.erase(tokens.begin());
	}

	BOOST_FOREACH(const ResolverSpec& resolver, resolvers) {
		if (!objName.IsEmpty() && objName != resolver.first)
			continue;

		if (objName.IsEmpty()) {
			CustomVarObject::Ptr dobj = dynamic_pointer_cast<CustomVarObject>(resolver.second);

			if (dobj) {
				Dictionary::Ptr vars = dobj->GetVars();

				if (vars && vars->Contains(macro)) {
					*result = vars->Get(macro);
					*recursive_macro = true;
					return true;
				}
			}
		}

		MacroResolver *mresolver = dynamic_cast<MacroResolver *>(resolver.second.get());

		if (mresolver && mresolver->ResolveMacro(boost::algorithm::join(tokens, "."), cr, result))
			return true;

		Value ref = resolver.second;
		bool valid = true;

		BOOST_FOREACH(const String& token, tokens) {
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

	return false;
}

Value MacroProcessor::InternalResolveMacrosShim(const std::vector<Value>& args, const ResolverList& resolvers,
    const CheckResult::Ptr& cr, const MacroProcessor::EscapeCallback& escapeFn, const Dictionary::Ptr& resolvedMacros,
    bool useResolvedMacros, int recursionLevel)
{
	if (args.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function"));

	String missingMacro;

	return MacroProcessor::InternalResolveMacros(args[0], resolvers, cr, &missingMacro, escapeFn,
	    resolvedMacros, useResolvedMacros, recursionLevel);
}

Value MacroProcessor::EvaluateFunction(const Function::Ptr& func, const ResolverList& resolvers,
    const CheckResult::Ptr& cr, const MacroProcessor::EscapeCallback& escapeFn,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros, int recursionLevel)
{
	Dictionary::Ptr resolvers_this = new Dictionary();

	BOOST_FOREACH(const ResolverSpec& resolver, resolvers) {
		resolvers_this->Set(resolver.first, resolver.second);
	}

	resolvers_this->Set("macro", new Function(boost::bind(&MacroProcessor::InternalResolveMacrosShim,
	    _1, boost::cref(resolvers), cr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros,
	    recursionLevel + 1)));

	ScriptFrame frame(resolvers_this);
	return func->Invoke();
}

Value MacroProcessor::InternalResolveMacros(const String& str, const ResolverList& resolvers,
    const CheckResult::Ptr& cr, String *missingMacro,
    const MacroProcessor::EscapeCallback& escapeFn, const Dictionary::Ptr& resolvedMacros,
    bool useResolvedMacros, int recursionLevel)
{
	CONTEXT("Resolving macros for string '" + str + "'");

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
			resolved_macro = EvaluateFunction(resolved_macro, resolvers, cr, escapeFn,
			    resolvedMacros, useResolvedMacros, recursionLevel);
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
				Array::Ptr resolved_arr = new Array();

				ObjectLock olock(arr);
				BOOST_FOREACH(const Value& value, arr) {
					if (value.IsScalar()) {
						resolved_arr->Add(InternalResolveMacros(value,
							resolvers, cr, missingMacro, EscapeCallback(), Dictionary::Ptr(),
							false, recursionLevel + 1));
					} else
						resolved_arr->Add(value);
				}

				resolved_macro = resolved_arr;
			} else if (resolved_macro.IsString()) {
				resolved_macro = InternalResolveMacros(resolved_macro,
					resolvers, cr, missingMacro, EscapeCallback(), Dictionary::Ptr(),
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
