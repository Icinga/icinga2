// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote/apifunction.hpp"

using namespace icinga;

INITIALIZE_ONCE_WITH_PRIORITY([]{
	ApiFunctionRegistry::GetInstance()->Freeze();
}, InitializePriority::FreezeNamespaces);

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
