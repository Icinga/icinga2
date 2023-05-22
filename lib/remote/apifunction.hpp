/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "remote/i2-remote.hpp"
#include "remote/messageorigin.hpp"
#include "base/registry.hpp"
#include "base/value.hpp"
#include "base/dictionary.hpp"
#include <vector>

namespace icinga
{

/**
 * A function available over the internal cluster API.
 *
 * @ingroup base
 */
class ApiFunction final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ApiFunction);

	typedef std::function<Value(const MessageOrigin::Ptr& origin, const Dictionary::Ptr&)> Callback;

	ApiFunction(Callback function);

	Value Invoke(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& arguments);

	static ApiFunction::Ptr GetByName(const String& name);
	static void Register(const String& name, const ApiFunction::Ptr& function);
	static void Unregister(const String& name);

private:
	Callback m_Callback;
};

/**
 * A registry for API functions.
 *
 * @ingroup base
 */
class ApiFunctionRegistry : public Registry<ApiFunctionRegistry, ApiFunction::Ptr>
{
public:
	static ApiFunctionRegistry *GetInstance();
};

#define REGISTER_APIFUNCTION(name, ns, callback) \
	INITIALIZE_ONCE([]() { \
		ApiFunction::Ptr func = new ApiFunction(callback); \
		ApiFunctionRegistry::GetInstance()->Register(#ns "::" #name, func); \
	})

}
