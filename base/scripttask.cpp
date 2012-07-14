#include "i2-base.h"

using namespace icinga;

ScriptTask::ScriptTask(const ScriptFunction::Ptr& function, const vector<Variant>& arguments, CompletionCallback callback)
	: AsyncTask(callback), m_Function(function), m_Arguments(arguments)
{ }

void ScriptTask::Run(void)
{
	m_Function->Invoke(GetSelf(), m_Arguments);
}

void ScriptTask::SetResult(const Variant& result)
{
	m_Result = result;
}

Variant ScriptTask::GetResult(void)
{
	return m_Result;
}