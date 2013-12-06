/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "config/configitem.h"
#include "config/configcompilercontext.h"
#include "base/application.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/debug.h"
#include <sstream>
#include <boost/foreach.hpp>

using namespace icinga;

boost::mutex ConfigItem::m_Mutex;
ConfigItem::ItemMap ConfigItem::m_Items;

/**
 * Constructor for the ConfigItem class.
 *
 * @param type The object type.
 * @param name The name of the item.
 * @param unit The unit of the item.
 * @param abstract Whether the item is a template.
 * @param exprl Expression list for the item.
 * @param parents Parent objects for the item.
 * @param debuginfo Debug information.
 */
ConfigItem::ConfigItem(const String& type, const String& name,
    bool abstract, const ExpressionList::Ptr& exprl,
    const std::vector<String>& parents, const DebugInfo& debuginfo)
	: m_Type(type), m_Name(name), m_Abstract(abstract), m_Validated(false),
	  m_ExpressionList(exprl), m_ParentNames(parents), m_DebugInfo(debuginfo)
{
}

/**
 * Retrieves the type of the configuration item.
 *
 * @returns The type.
 */
String ConfigItem::GetType(void) const
{
	return m_Type;
}

/**
 * Retrieves the name of the configuration item.
 *
 * @returns The name.
 */
String ConfigItem::GetName(void) const
{
	return m_Name;
}

/**
 * Checks whether the item is abstract.
 *
 * @returns true if the item is abstract, false otherwise.
 */
bool ConfigItem::IsAbstract(void) const
{
	return m_Abstract;
}

/**
 * Retrieves the debug information for the configuration item.
 *
 * @returns The debug information.
 */
DebugInfo ConfigItem::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

/**
 * Retrieves the expression list for the configuration item.
 *
 * @returns The expression list.
 */
ExpressionList::Ptr ConfigItem::GetExpressionList(void) const
{
	return m_ExpressionList;
}

void ConfigItem::Link(void)
{
	ObjectLock olock(this);

	if (m_LinkedExpressionList)
		return;

	m_LinkedExpressionList = make_shared<ExpressionList>();

	BOOST_FOREACH(const String& name, m_ParentNames) {
		ConfigItem::Ptr parent = ConfigItem::GetObject(m_Type, name);

		if (!parent) {
			std::ostringstream message;
			message << "Parent object '" << name << "' does not"
			    " exist (" << m_DebugInfo << ")";
			ConfigCompilerContext::GetInstance()->AddMessage(true, message.str());
		} else {
			parent->Link();

			ExpressionList::Ptr pexprl = parent->m_LinkedExpressionList;
			m_LinkedExpressionList->AddExpression(Expression("", OperatorExecute, pexprl, m_DebugInfo));
		}
	}

	m_LinkedExpressionList->AddExpression(Expression("", OperatorExecute, m_ExpressionList, m_DebugInfo));
}

ExpressionList::Ptr ConfigItem::GetLinkedExpressionList(void)
{
	if (!m_LinkedExpressionList)
		Link();

	return m_LinkedExpressionList;
}

Dictionary::Ptr ConfigItem::GetProperties(void)
{
	if (!m_Properties) {
		m_Properties = make_shared<Dictionary>();
		GetLinkedExpressionList()->Execute(m_Properties);
	}

	return m_Properties;
}

/**
 * Commits the configuration item by creating a DynamicObject
 * object.
 *
 * @returns The DynamicObject that was created/updated.
 */
DynamicObject::Ptr ConfigItem::Commit(void)
{
	ASSERT(!OwnsLock());

#ifdef _DEBUG
	Log(LogDebug, "base", "Commit called for ConfigItem Type=" + GetType() + ", Name=" + GetName());
#endif /* _DEBUG */

	/* Make sure the type is valid. */
	DynamicType::Ptr dtype = DynamicType::GetByName(GetType());

	if (!dtype)
		BOOST_THROW_EXCEPTION(std::runtime_error("Type '" + GetType() + "' does not exist."));

	if (dtype->GetObject(GetName()))
	    BOOST_THROW_EXCEPTION(std::runtime_error("An object with type '" + GetType() + "' and name '" + GetName() + "' already exists."));

	if (IsAbstract())
		return DynamicObject::Ptr();

	Dictionary::Ptr properties = GetProperties();

	DynamicObject::Ptr dobj = dtype->CreateObject(properties);
	dobj->Register();

	m_Object = dobj;
	
	return dobj;
}

DynamicObject::Ptr ConfigItem::GetObject(void) const
{
	return m_Object;
}

/**
 * Registers the configuration item.
 */
void ConfigItem::Register(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	m_Items[std::make_pair(m_Type, m_Name)] = GetSelf();
}

/**
 * Retrieves a configuration item by type and name.
 *
 * @param type The type of the ConfigItem that is to be looked up.
 * @param name The name of the ConfigItem that is to be looked up.
 * @returns The configuration item.
 */
ConfigItem::Ptr ConfigItem::GetObject(const String& type, const String& name)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	ConfigItem::ItemMap::iterator it;

	it = m_Items.find(std::make_pair(type, name));

	if (it != m_Items.end())
		return it->second;

	return ConfigItem::Ptr();
}

void ConfigItem::ValidateItem(void)
{
	if (m_Validated)
		return;

	ConfigType::Ptr ctype = ConfigType::GetByName(GetType());

	if (!ctype) {
		ConfigCompilerContext::GetInstance()->AddMessage(false, "No validation type found for object '" + GetName() + "' of type '" + GetType() + "'");

		return;
	}

	ctype->ValidateItem(GetSelf());

	m_Validated = true;
}

bool ConfigItem::ActivateItems(bool validateOnly)
{
	if (ConfigCompilerContext::GetInstance()->HasErrors())
		return false;

	if (ConfigCompilerContext::GetInstance()->HasErrors())
		return false;

	Log(LogInformation, "config", "Validating config items (step 1)...");

	ThreadPool tp(32);
	
	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		tp.Post(boost::bind(&ConfigItem::ValidateItem, kv.second));
	}
	
	tp.Join();

	if (ConfigCompilerContext::GetInstance()->HasErrors())
		return false;

	Log(LogInformation, "config", "Comitting config items");

	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		tp.Post(boost::bind(&ConfigItem::Commit, kv.second));
	}
	
	tp.Join();
	
	std::vector<DynamicObject::Ptr> objects;
	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		DynamicObject::Ptr object = kv.second->GetObject();

		if (object)
			objects.push_back(object);
	}
	
	Log(LogInformation, "config", "Triggering OnConfigLoaded signal for config items");
	
	BOOST_FOREACH(const DynamicObject::Ptr& object, objects) {
		tp.Post(boost::bind(&DynamicObject::OnConfigLoaded, object));
	}
	
	tp.Join();

	Log(LogInformation, "config", "Validating config items (step 2)...");

	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		tp.Post(boost::bind(&ConfigItem::ValidateItem, kv.second));
	}

	tp.Join();
	
	if (ConfigCompilerContext::GetInstance()->HasErrors())
		return false;

	if (validateOnly)
		return true;

	/* restore the previous program state */
	DynamicObject::RestoreObjects(Application::GetStatePath());

	Log(LogInformation, "config", "Triggering Start signal for config items");
	
	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			if (object->IsActive())
				continue;

#ifdef _DEBUG
			Log(LogDebug, "config", "Activating object '" + object->GetName() + "' of type '" + object->GetType()->GetName() + "'");
#endif /* _DEBUG */
			tp.Post(boost::bind(&DynamicObject::Start, object));
		}
	}
	
	tp.Join();
	
#ifdef _DEBUG
	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			ASSERT(object->IsActive());
		}
	}
#endif /* _DEBUG */

	Log(LogInformation, "config", "Activated all objects.");

	return true;
}

void ConfigItem::DiscardItems(void)
{
	m_Items.clear();
}
