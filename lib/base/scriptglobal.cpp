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

#include "base/scriptglobal.hpp"
#include "base/singleton.hpp"
#include "base/logger.hpp"
#include "base/stdiostream.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/split.hpp>
#include <fstream>

using namespace icinga;

Dictionary::Ptr ScriptGlobal::m_Globals = new Dictionary();

Value ScriptGlobal::Get(const String& name, const Value *defaultValue)
{
	Value result;

	if (!m_Globals->Get(name, &result)) {
		if (defaultValue)
			return *defaultValue;

		BOOST_THROW_EXCEPTION(std::invalid_argument("Tried to access undefined script variable '" + name + "'"));
	}

	return result;
}

void ScriptGlobal::Set(const String& name, const Value& value)
{
	std::vector<String> tokens;
	boost::algorithm::split(tokens, name, boost::is_any_of("."));

	if (tokens.empty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Name must not be empty"));

	{
		ObjectLock olock(m_Globals);

		Dictionary::Ptr parent = m_Globals;

		for (std::vector<String>::size_type i = 0; i < tokens.size(); i++) {
			const String& token = tokens[i];

			if (i + 1 != tokens.size()) {
				Value vparent;

				if (!parent->Get(token, &vparent)) {
					Dictionary::Ptr dict = new Dictionary();
					parent->Set(token, dict);
					parent = dict;
				} else {
					parent = vparent;
				}
			}
		}

		parent->Set(tokens[tokens.size() - 1], value);
	}
}

bool ScriptGlobal::Exists(const String& name)
{
	return m_Globals->Contains(name);
}

Dictionary::Ptr ScriptGlobal::GetGlobals()
{
	return m_Globals;
}

void ScriptGlobal::WriteToFile(const String& filename)
{
	Log(LogInformation, "ScriptGlobal")
		<< "Dumping variables to file '" << filename << "'";

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(filename + ".XXXXXX", 0600, fp);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + tempFilename + "' file"));

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	ObjectLock olock(m_Globals);
	for (const Dictionary::Pair& kv : m_Globals) {
		Dictionary::Ptr persistentVariable = new Dictionary();

		persistentVariable->Set("name", kv.first);

		Value value = kv.second;

		if (value.IsObject())
			value = Convert::ToString(value);

		persistentVariable->Set("value", value);

		String json = JsonEncode(persistentVariable);

		NetString::WriteStringToStream(sfp, json);
	}

	sfp->Close();

	fp.close();

#ifdef _WIN32
	_unlink(filename.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), filename.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(tempFilename));
	}
}

