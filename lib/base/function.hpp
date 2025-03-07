/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef FUNCTION_H
#define FUNCTION_H

#include "base/i2-base.hpp"
#include "base/function-ti.hpp"
#include "base/value.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptglobal.hpp"
#include <vector>

namespace icinga
{

/**
 * A script function that can be used to execute a script task.
 *
 * @ingroup base
 */
class Function final : public ObjectImpl<Function>
{
public:
	DECLARE_OBJECT(Function);

	typedef std::function<Value (const std::vector<Value>& arguments)> Callback;

	template<typename F>
	Function(const String& name, F function, const std::vector<String>& args = std::vector<String>(),
		bool hasClosedThis = false, Value closedThis = Value(),
		bool side_effect_free = false, bool deprecated = false)
		: Function(name, WrapFunction(function), args, hasClosedThis, std::move(closedThis), side_effect_free, deprecated)
	{ }

	Value Invoke(const std::vector<Value>& arguments = std::vector<Value>());
	Value InvokeThis(const Value& otherThis, const std::vector<Value>& arguments = std::vector<Value>());

	bool IsSideEffectFree() const
	{
		return GetSideEffectFree();
	}

	bool IsDeprecated() const
	{
		return GetDeprecated();
	}

	static Object::Ptr GetPrototype();

	Object::Ptr Clone() const override;

private:
	Callback m_Callback;
	bool m_HasClosedThis;
	Value m_ClosedThis;

	Function(const String& name, Callback function, const std::vector<String>& args,
		bool hasClosedThis, Value closedThis,
		bool side_effect_free, bool deprecated);
};

/* Ensure that the priority is lower than the basic namespace initialization in scriptframe.cpp. */
#define REGISTER_FUNCTION(ns, name, callback, args) \
	INITIALIZE_ONCE_WITH_PRIORITY([]() { \
		Function::Ptr sf = new icinga::Function(#ns "#" #name, callback, String(args).Split(":"), false); \
		Namespace::Ptr nsp = ScriptGlobal::Get(#ns); \
		nsp->Set(#name, sf, true); \
	}, InitializePriority::RegisterFunctions)

#define REGISTER_SAFE_FUNCTION(ns, name, callback, args) \
	INITIALIZE_ONCE_WITH_PRIORITY([]() { \
		Function::Ptr sf = new icinga::Function(#ns "#" #name, callback, String(args).Split(":"), true); \
		Namespace::Ptr nsp = ScriptGlobal::Get(#ns); \
		nsp->Set(#name, sf, true); \
	}, InitializePriority::RegisterFunctions)

#define REGISTER_FUNCTION_NONCONST(ns, name, callback, args) \
	INITIALIZE_ONCE_WITH_PRIORITY([]() { \
		Function::Ptr sf = new icinga::Function(#ns "#" #name, callback, String(args).Split(":"), false); \
		Namespace::Ptr nsp = ScriptGlobal::Get(#ns); \
		nsp->Set(#name, sf, false); \
	}, InitializePriority::RegisterFunctions)

#define REGISTER_SAFE_FUNCTION_NONCONST(ns, name, callback, args) \
	INITIALIZE_ONCE_WITH_PRIORITY([]() { \
		Function::Ptr sf = new icinga::Function(#ns "#" #name, callback, String(args).Split(":"), true); \
		Namespace::Ptr nsp = ScriptGlobal::Get(#ns); \
		nsp->SetAttribute(#name, sf, false); \
	}, InitializePriority::RegisterFunctions)

}

#endif /* FUNCTION_H */
