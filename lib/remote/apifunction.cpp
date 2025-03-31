/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/apifunction.hpp"
#include "base/singleton.hpp"

using namespace icinga;

ApiFunction::ApiFunction(const char* name, Callback function)
	: m_Name(name), m_Callback(std::move(function))
{ }

Value ApiFunction::Invoke(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& arguments)
{
	return m_Callback(origin, arguments);
}

ApiFunction::Ptr ApiFunction::GetByName(const String& name)
{
	return ApiFunctionRegistry::GetInstance()->GetItem(name);
}

void ApiFunction::Register(const String& name, const ApiFunction::Ptr& function)
{
	ApiFunctionRegistry::GetInstance()->Register(name, function);
}

ApiFunctionRegistry *ApiFunctionRegistry::GetInstance()
{
	return Singleton<ApiFunctionRegistry>::GetInstance();
}
