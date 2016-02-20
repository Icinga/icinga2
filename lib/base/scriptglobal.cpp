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

#include "base/scriptglobal.hpp"
#include "base/singleton.hpp"
#include "base/logger.hpp"
#include "base/stdiostream.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>
#include <fstream>

using namespace icinga;

Dictionary::Ptr ScriptGlobal::m_Globals = new Dictionary();

Value ScriptGlobal::Get(const String& name, const Value *defaultValue)
{
	if (!m_Globals->Contains(name)) {
		if (defaultValue)
			return *defaultValue;

		BOOST_THROW_EXCEPTION(std::invalid_argument("Tried to access undefined script variable '" + name + "'"));
	}

	return m_Globals->Get(name);
}

void ScriptGlobal::Set(const String& name, const Value& value)
{
	m_Globals->Set(name, value);
}

bool ScriptGlobal::Exists(const String& name)
{
	return m_Globals->Contains(name);
}

Dictionary::Ptr ScriptGlobal::GetGlobals(void)
{
	return m_Globals;
}

void ScriptGlobal::WriteToFile(const String& filename)
{
	Log(LogInformation, "ScriptGlobal")
		<< "Dumping variables to file '" << filename << "'";

	String tempFilename = filename + ".tmp";

	std::fstream fp;
	fp.open(tempFilename.CStr(), std::ios_base::out);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + tempFilename + "' file"));

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	ObjectLock olock(m_Globals);
	BOOST_FOREACH(const Dictionary::Pair& kv, m_Globals) {
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

