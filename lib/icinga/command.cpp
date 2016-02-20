/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "icinga/command.hpp"
#include "icinga/command.tcpp"
#include "icinga/macroprocessor.hpp"
#include "base/exception.hpp"
#include "base/objectlock.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Command);

void Command::Validate(int types, const ValidationUtils& utils)
{
	ObjectImpl<Command>::Validate(types, utils);

	Dictionary::Ptr arguments = GetArguments();

	if (!(types & FAConfig))
		return;

	if (arguments) {
		if (!GetCommandLine().IsObjectType<Array>())
			BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("command"), "Attribute 'command' must be an array if the 'arguments' attribute is set."));

		ObjectLock olock(arguments);
		BOOST_FOREACH(const Dictionary::Pair& kv, arguments) {
			const Value& arginfo = kv.second;
			Value argval;

			if (arginfo.IsObjectType<Dictionary>()) {
				Dictionary::Ptr argdict = arginfo;

				if (argdict->Contains("value")) {
					Value argvalue = argdict->Get("value");

					if (argvalue.IsString() && !MacroProcessor::ValidateMacroString(argvalue))
						BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("arguments")(kv.first)("value"), "Validation failed: Closing $ not found in macro format string '" + argvalue + "'."));
				}

				if (argdict->Contains("set_if")) {
					Value argsetif = argdict->Get("set_if");

					if (argsetif.IsString() && !MacroProcessor::ValidateMacroString(argsetif))
						BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("arguments")(kv.first)("set_if"), "Closing $ not found in macro format string '" + argsetif + "'."));
				}
			} else if (arginfo.IsString()) {
				if (!MacroProcessor::ValidateMacroString(arginfo))
					BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("arguments")(kv.first), "Closing $ not found in macro format string '" + arginfo + "'."));
			}
		}
	}

	Dictionary::Ptr env = GetEnv();

	if (env) {
		ObjectLock olock(env);
		BOOST_FOREACH(const Dictionary::Pair& kv, env) {
			const Value& envval = kv.second;

			if (!envval.IsString() || envval.IsEmpty())
				continue;

			if (!MacroProcessor::ValidateMacroString(envval))
				BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("env")(kv.first), "Closing $ not found in macro format string '" + envval + "'."));
		}
	}
}
