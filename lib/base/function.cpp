/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/function.hpp"
#include "base/function-ti.cpp"
#include "base/array.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(Function, Function::GetPrototype());

Function::Function(const String& name, Callback function, const std::vector<String>& args,
	bool hasClosedThis, Value closedThis,
	bool side_effect_free, bool deprecated)
	: m_Callback(std::move(function)), m_HasClosedThis(hasClosedThis), m_ClosedThis(std::move(closedThis))
{
	SetName(name, true);
	SetSideEffectFree(side_effect_free, true);
	SetDeprecated(deprecated, true);
	SetArguments(Array::FromVector(args), true);
}

Value Function::Invoke(const std::vector<Value>& arguments)
{
	if (m_HasClosedThis) {
		return InvokeThis(m_ClosedThis, arguments);
	}

	ScriptFrame frame(false);
	return m_Callback(arguments);
}

Value Function::InvokeThis(const Value& otherThis, const std::vector<Value>& arguments)
{
	ScriptFrame frame(false, otherThis);
	return m_Callback(arguments);
}

Object::Ptr Function::Clone() const
{
	return const_cast<Function *>(this);
}
