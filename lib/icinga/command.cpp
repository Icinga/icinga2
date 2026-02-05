// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/command.hpp"
#include "icinga/command-ti.cpp"
#include "icinga/macroprocessor.hpp"
#include "base/exception.hpp"
#include "base/objectlock.hpp"

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
			BOOST_THROW_EXCEPTION(ValidationError(this, { "command" }, "Attribute 'command' must be an array if the 'arguments' attribute is set."));

		ObjectLock olock(arguments);
		for (const Dictionary::Pair& kv : arguments) {
			const Value& arginfo = kv.second;
			Value argval;

			if (arginfo.IsObjectType<Dictionary>()) {
				Dictionary::Ptr argdict = arginfo;

				if (argdict->Contains("value")) {
					Value argvalue = argdict->Get("value");

					if (argvalue.IsString() && !MacroProcessor::ValidateMacroString(argvalue))
						BOOST_THROW_EXCEPTION(ValidationError(this, { "arguments", kv.first, "value" }, "Closing $ not found in macro format string '" + argvalue + "'."));
				}

				if (argdict->Contains("set_if")) {
					Value argsetif = argdict->Get("set_if");

					if (argsetif.IsString() && !MacroProcessor::ValidateMacroString(argsetif))
						BOOST_THROW_EXCEPTION(ValidationError(this, { "arguments", kv.first, "set_if" }, "Closing $ not found in macro format string '" + argsetif + "'."));
				}
			} else if (arginfo.IsString()) {
				if (!MacroProcessor::ValidateMacroString(arginfo))
					BOOST_THROW_EXCEPTION(ValidationError(this, { "arguments", kv.first }, "Closing $ not found in macro format string '" + arginfo + "'."));
			}
		}
	}

	Dictionary::Ptr env = GetEnv();

	if (env) {
		ObjectLock olock(env);
		for (const Dictionary::Pair& kv : env) {
			const Value& envval = kv.second;

			if (!envval.IsString() || envval.IsEmpty())
				continue;

			if (!MacroProcessor::ValidateMacroString(envval))
				BOOST_THROW_EXCEPTION(ValidationError(this, { "env", kv.first }, "Closing $ not found in macro format string '" + envval + "'."));
		}
	}
}
