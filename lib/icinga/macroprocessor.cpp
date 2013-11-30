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

#include "icinga/macroprocessor.h"
#include "icinga/macroresolver.h"
#include "base/utility.h"
#include "base/array.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/context.h"
#include <boost/foreach.hpp>

using namespace icinga;

Value MacroProcessor::ResolveMacros(const Value& str, const std::vector<MacroResolver::Ptr>& resolvers,
	const CheckResult::Ptr& cr, const MacroProcessor::EscapeCallback& escapeFn, const Array::Ptr& escapeMacros)
{
	Value result;

	if (str.IsEmpty())
		return Empty;

	if (str.IsScalar()) {
		result = InternalResolveMacros(str, resolvers, cr, escapeFn, escapeMacros);
	} else if (str.IsObjectType<Array>()) {
		Array::Ptr resultArr = make_shared<Array>();
		Array::Ptr arr = str;

		ObjectLock olock(arr);

		BOOST_FOREACH(const Value& arg, arr) {
			/* Note: don't escape macros here. */
			resultArr->Add(InternalResolveMacros(arg, resolvers, cr, EscapeCallback(), Array::Ptr()));
		}

		result = resultArr;
	} else {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Command is not a string or array."));
	}

	return result;
}

bool MacroProcessor::ResolveMacro(const String& macro, const std::vector<MacroResolver::Ptr>& resolvers,
    const CheckResult::Ptr& cr, String *result)
{
	CONTEXT("Resolving macro '" + macro + "'");

	BOOST_FOREACH(const MacroResolver::Ptr& resolver, resolvers) {
		if (resolver->ResolveMacro(macro, cr, result))
			return true;
	}

	return false;
}


String MacroProcessor::InternalResolveMacros(const String& str, const std::vector<MacroResolver::Ptr>& resolvers,
	const CheckResult::Ptr& cr, const MacroProcessor::EscapeCallback& escapeFn, const Array::Ptr& escapeMacros)
{
	CONTEXT("Resolving macros for string '" + str + "'");

	size_t offset, pos_first, pos_second;
	offset = 0;

	String result = str;
	while ((pos_first = result.FindFirstOf("$", offset)) != String::NPos) {
		pos_second = result.FindFirstOf("$", pos_first + 1);

		if (pos_second == String::NPos)
			BOOST_THROW_EXCEPTION(std::runtime_error("Closing $ not found in macro format string."));

		String name = result.SubStr(pos_first + 1, pos_second - pos_first - 1);

		String resolved_macro;
		bool found = ResolveMacro(name, resolvers, cr, &resolved_macro);

		/* $$ is an escape sequence for $. */
		if (name.IsEmpty()) {
			resolved_macro = "$";
			found = true;
		}

		if (!found)
			Log(LogWarning, "icinga", "Macro '" + name + "' is not defined.");

		if (escapeFn && escapeMacros) {
			bool escape = false;

			ObjectLock olock(escapeMacros);
			BOOST_FOREACH(const String& escapeMacro, escapeMacros) {
				if (escapeMacro == name) {
					escape = true;
					break;
				}
			}

			if (escape)
				resolved_macro = escapeFn(resolved_macro);
		}

		result.Replace(pos_first, pos_second - pos_first + 1, resolved_macro);
		offset = pos_first + resolved_macro.GetLength();
	}

	return result;
}
