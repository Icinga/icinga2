/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/apiaction.hpp"
#include "base/singleton.hpp"

using namespace icinga;

ApiAction::ApiAction(std::vector<String> types, Callback action)
	: m_Types(std::move(types)), m_Callback(std::move(action))
{ }

Value ApiAction::Invoke(const ConfigObject::Ptr& target, const Dictionary::Ptr& params)
{
	return m_Callback(target, params);
}

const std::vector<String>& ApiAction::GetTypes() const
{
	return m_Types;
}

ApiAction::Ptr ApiAction::GetByName(const String& name)
{
	return ApiActionRegistry::GetInstance()->GetItem(name);
}

void ApiAction::Register(const String& name, const ApiAction::Ptr& action)
{
	ApiActionRegistry::GetInstance()->Register(name, action);
}

void ApiAction::Unregister(const String& name)
{
	ApiActionRegistry::GetInstance()->Unregister(name);
}

ApiActionRegistry *ApiActionRegistry::GetInstance()
{
	return Singleton<ApiActionRegistry>::GetInstance();
}
