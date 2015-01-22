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

#include "base/scriptvariable.hpp"
#include "base/singleton.hpp"
#include "base/logger.hpp"
#include "base/stdiostream.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include <boost/foreach.hpp>
#include <fstream>

using namespace icinga;

ScriptVariable::ScriptVariable(const Value& data)
	: m_Data(data), m_Constant(false)
{ }

ScriptVariable::Ptr ScriptVariable::GetByName(const String& name)
{
	return ScriptVariableRegistry::GetInstance()->GetItem(name);
}

void ScriptVariable::SetConstant(bool constant)
{
	m_Constant = constant;
}

bool ScriptVariable::IsConstant(void) const
{
	return m_Constant;
}

void ScriptVariable::SetData(const Value& data)
{
	m_Data = data;
}

Value ScriptVariable::GetData(void) const
{
	return m_Data;
}

Value ScriptVariable::Get(const String& name, const Value *defaultValue)
{
	ScriptVariable::Ptr sv = GetByName(name);

	if (!sv) {
		if (defaultValue)
			return *defaultValue;

		BOOST_THROW_EXCEPTION(std::invalid_argument("Tried to access undefined script variable '" + name + "'"));
	}

	return sv->GetData();
}

ScriptVariable::Ptr ScriptVariable::Set(const String& name, const Value& value, bool overwrite, bool make_const)
{
	ScriptVariable::Ptr sv = GetByName(name);

	if (!sv) {
		sv = new ScriptVariable(value);
		ScriptVariableRegistry::GetInstance()->Register(name, sv);
	} else if (overwrite) {
		if (sv->IsConstant())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Tried to modify read-only script variable '" + name + "'"));

		sv->SetData(value);
	}

	if (make_const)
		sv->SetConstant(true);

	return sv;
}

void ScriptVariable::Unregister(const String& name)
{
	ScriptVariableRegistry::GetInstance()->Unregister(name);
}

void ScriptVariable::WriteVariablesFile(const String& filename)
{
	Log(LogInformation, "ScriptVariable")
		<< "Dumping variables to file '" << filename << "'";

	String tempFilename = filename + ".tmp";

	std::fstream fp;
	fp.open(tempFilename.CStr(), std::ios_base::out);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + tempFilename + "' file"));

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	BOOST_FOREACH(const ScriptVariableRegistry::ItemMap::value_type& kv, ScriptVariableRegistry::GetInstance()->GetItems()) {
		Dictionary::Ptr persistentVariable = new Dictionary();

		persistentVariable->Set("name", kv.first);

		ScriptVariable::Ptr sv = kv.second;
		Value value = sv->GetData();

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

ScriptVariableRegistry *ScriptVariableRegistry::GetInstance(void)
{
	return Singleton<ScriptVariableRegistry>::GetInstance();
}

