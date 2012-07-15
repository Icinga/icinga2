#include "i2-base.h"

using namespace icinga;

ScriptTask::ScriptTask(const ScriptFunction::Ptr& function, const vector<Variant>& arguments)
	: AsyncTask<ScriptTask, Variant>(), m_Function(function), m_Arguments(arguments)
{ }

void ScriptTask::Run(void)
{
	m_Function->Invoke(GetSelf(), m_Arguments);
}
