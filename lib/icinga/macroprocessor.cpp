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

#include "i2-icinga.h"

using namespace icinga;

String MacroProcessor::ResolveMacros(const String& str, const vector<Dictionary::Ptr>& macroDicts)
{
	size_t offset, pos_first, pos_second;

	offset = 0;

	String result = str;
	while ((pos_first = result.FindFirstOf("$", offset)) != String::NPos) {
		pos_second = result.FindFirstOf("$", pos_first + 1);

		if (pos_second == String::NPos)
			BOOST_THROW_EXCEPTION(runtime_error("Closing $ not found in macro format String."));

		String name = result.SubStr(pos_first + 1, pos_second - pos_first - 1);
		String value;
		bool resolved = false;

		BOOST_FOREACH(const Dictionary::Ptr& macroDict, macroDicts) {
			if (!macroDict || !macroDict->Contains(name))
				continue;

			String value = macroDict->Get(name);
			result.Replace(pos_first, pos_second - pos_first + 1, value);
			offset = pos_first + value.GetLength();

			resolved = true;
			break;
		}

		if (!resolved)
			BOOST_THROW_EXCEPTION(runtime_error("Macro '" + name + "' is not defined."));
	}

	return result;
}

Dictionary::Ptr MacroProcessor::MakeEnvironment(const vector<Dictionary::Ptr>& dicts)
{
	Dictionary::Ptr result = boost::make_shared<Dictionary>();

	BOOST_REVERSE_FOREACH(const Dictionary::Ptr& dict, dicts) {
		String key;
		Value value;
		BOOST_FOREACH(tie(key, value), dict) {
			if (!value.IsScalar())
				continue;

			result->Set(key, value);
		}
	}

	return result;
}
