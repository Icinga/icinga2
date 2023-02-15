/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/atomic-file.hpp"
#include "base/scriptglobal.hpp"
#include "base/singleton.hpp"
#include "base/logger.hpp"
#include "base/stdiostream.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"
#include "base/namespace.hpp"
#include "base/utility.hpp"
#include <fstream>

using namespace icinga;

Namespace::Ptr ScriptGlobal::m_Globals = new Namespace();

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

void ScriptGlobal::Set(const String& name, const Value& value, bool overrideFrozen)
{
	std::vector<String> tokens = name.Split(".");

	if (tokens.empty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Name must not be empty"));

	{
		ObjectLock olock(m_Globals);

		Namespace::Ptr parent = m_Globals;

		for (std::vector<String>::size_type i = 0; i < tokens.size(); i++) {
			const String& token = tokens[i];

			if (i + 1 != tokens.size()) {
				Value vparent;

				if (!parent->Get(token, &vparent)) {
					Namespace::Ptr dict = new Namespace();
					parent->Set(token, dict);
					parent = dict;
				} else {
					parent = vparent;
				}
			}
		}

		parent->SetFieldByName(tokens[tokens.size() - 1], value, overrideFrozen, DebugInfo());
	}
}

void ScriptGlobal::SetConst(const String& name, const Value& value)
{
	GetGlobals()->SetAttribute(name, new ConstEmbeddedNamespaceValue(value));
}

bool ScriptGlobal::Exists(const String& name)
{
	return m_Globals->Contains(name);
}

Namespace::Ptr ScriptGlobal::GetGlobals()
{
	return m_Globals;
}

void ScriptGlobal::WriteToFile(const String& filename)
{
	Log(LogInformation, "ScriptGlobal")
		<< "Dumping variables to file '" << filename << "'";

	AtomicFile fp (filename, 0600);
	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	ObjectLock olock(m_Globals);
	for (const Namespace::Pair& kv : m_Globals) {
		Value value = kv.second->Get();

		if (value.IsObject())
			value = Convert::ToString(value);

		Dictionary::Ptr persistentVariable = new Dictionary({
			{ "name", kv.first },
			{ "value", value }
		});

		String json = JsonEncode(persistentVariable);

		NetString::WriteStringToStream(sfp, json);
	}

	sfp->Close();
	fp.Commit();
}

