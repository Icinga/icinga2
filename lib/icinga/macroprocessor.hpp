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

#ifndef MACROPROCESSOR_H
#define MACROPROCESSOR_H

#include "icinga/i2-icinga.hpp"
#include "icinga/checkable.hpp"
#include "base/value.hpp"
#include <vector>

namespace icinga
{

/**
 * Resolves macros.
 *
 * @ingroup icinga
 */
class MacroProcessor
{
public:
	typedef std::function<Value (const Value&)> EscapeCallback;
	typedef std::pair<String, Object::Ptr> ResolverSpec;
	typedef std::vector<ResolverSpec> ResolverList;

	static Value ResolveMacros(const Value& str, const ResolverList& resolvers,
		const CheckResult::Ptr& cr = nullptr, String *missingMacro = nullptr,
		const EscapeCallback& escapeFn = EscapeCallback(),
		const Dictionary::Ptr& resolvedMacros = nullptr,
		bool useResolvedMacros = false, int recursionLevel = 0);

	static Value ResolveArguments(const Value& command, const Dictionary::Ptr& arguments,
		const MacroProcessor::ResolverList& resolvers, const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros, int recursionLevel = 0);

	static bool ValidateMacroString(const String& macro);
	static void ValidateCustomVars(const ConfigObject::Ptr& object, const Dictionary::Ptr& value);

private:
	MacroProcessor();

	static bool ResolveMacro(const String& macro, const ResolverList& resolvers,
		const CheckResult::Ptr& cr, Value *result, bool *recursive_macro);
	static Value InternalResolveMacros(const String& str,
		const ResolverList& resolvers, const CheckResult::Ptr& cr,
		String *missingMacro, const EscapeCallback& escapeFn,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros,
		int recursionLevel = 0);
	static Value EvaluateFunction(const Function::Ptr& func, const ResolverList& resolvers,
		const CheckResult::Ptr& cr, const MacroProcessor::EscapeCallback& escapeFn,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros, int recursionLevel);

	static void AddArgumentHelper(const Array::Ptr& args, const String& key, const String& value,
		bool add_key, bool add_value);
	static Value EscapeMacroShellArg(const Value& value);

};

}

#endif /* MACROPROCESSOR_H */
