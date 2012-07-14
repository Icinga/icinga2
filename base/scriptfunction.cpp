#include "i2-base.h"

using namespace icinga;

map<string, ScriptFunction::Ptr> ScriptFunction::m_Functions;

ScriptFunction::ScriptFunction(const Callback& function)
	: m_Callback(function)
{ }

void ScriptFunction::Register(const string& name, const ScriptFunction::Ptr& function)
{
	m_Functions[name] = function;
}

void ScriptFunction::Unregister(const string& name)
{
	m_Functions.erase(name);
}

ScriptFunction::Ptr ScriptFunction::GetByName(const string& name)
{
	map<string, ScriptFunction::Ptr>::iterator it;

	it = m_Functions.find(name);

	if (it == m_Functions.end())
		return ScriptFunction::Ptr();

	return it->second;
}

void ScriptFunction::Invoke(const ScriptTask::Ptr& task, const vector<Variant>& arguments)
{
	m_Callback(task, arguments);
}
