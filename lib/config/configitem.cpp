/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "config/configitem.hpp"
#include "config/configcompilercontext.hpp"
#include "config/applyrule.hpp"
#include "config/objectrule.hpp"
#include "config/configtype.hpp"
#include "base/application.hpp"
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"
#include "base/debug.hpp"
#include "base/workqueue.hpp"
#include "base/exception.hpp"
#include "base/stdiostream.hpp"
#include "base/netstring.hpp"
#include "base/serializer.hpp"
#include "base/json.hpp"
#include "base/configerror.hpp"
#include <sstream>
#include <fstream>
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
 * @param debuginfo Debug information.
 */
ConfigItem::ConfigItem(const String& type, const String& name,
    bool abstract, const Expression::Ptr& exprl,
    const DebugInfo& debuginfo, const Object::Ptr& scope,
    const String& zone)
	: m_Type(type), m_Name(name), m_Abstract(abstract),
	  m_ExpressionList(exprl), m_DebugInfo(debuginfo),
	  m_Scope(scope), m_Zone(zone)
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

Object::Ptr ConfigItem::GetScope(void) const
{
	return m_Scope;
}

/**
 * Retrieves the expression list for the configuration item.
 *
 * @returns The expression list.
 */
Expression::Ptr ConfigItem::GetExpressionList(void) const
{
	return m_ExpressionList;
}

/**
 * Commits the configuration item by creating a DynamicObject
 * object.
 *
 * @returns The DynamicObject that was created/updated.
 */
DynamicObject::Ptr ConfigItem::Commit(bool discard)
{
	ASSERT(!OwnsLock());

#ifdef _DEBUG
	Log(LogDebug, "ConfigItem")
	    << "Commit called for ConfigItem Type=" << GetType() << ", Name=" << GetName();
#endif /* _DEBUG */

	/* Make sure the type is valid. */
	Type::Ptr type = Type::GetByName(GetType());

	if (!type || !Type::GetByName("DynamicObject")->IsAssignableFrom(type))
		BOOST_THROW_EXCEPTION(ConfigError("Type '" + GetType() + "' does not exist."));

	if (IsAbstract())
		return DynamicObject::Ptr();

	DynamicObject::Ptr dobj = static_pointer_cast<DynamicObject>(type->Instantiate());

	dobj->SetDebugInfo(m_DebugInfo);
	dobj->SetTypeName(m_Type);
	dobj->SetZone(m_Zone);

	Dictionary::Ptr locals = make_shared<Dictionary>();
	locals->Set("__parent", m_Scope);
	m_Scope.reset();
	locals->Set("name", m_Name);

	dobj->SetParentScope(locals);

	DebugHint debugHints;

	try {
		m_ExpressionList->Evaluate(dobj, &debugHints);
	} catch (const ConfigError& ex) {
		const DebugInfo *di = boost::get_error_info<errinfo_debuginfo>(ex);
		ConfigCompilerContext::GetInstance()->AddMessage(true, ex.what(), di ? *di : DebugInfo());
	} catch (const std::exception& ex) {
		ConfigCompilerContext::GetInstance()->AddMessage(true, DiagnosticInformation(ex));
	}

	if (discard)
		m_ExpressionList.reset();

	dobj->SetParentScope(Dictionary::Ptr());

	String name = m_Name;

	shared_ptr<NameComposer> nc = dynamic_pointer_cast<NameComposer>(type);

	if (nc) {
		name = nc->MakeName(m_Name, dobj);

		if (name.IsEmpty())
			BOOST_THROW_EXCEPTION(std::runtime_error("Could not determine name for object"));
	}

	if (name != m_Name)
		dobj->SetShortName(m_Name);

	dobj->SetName(name);

	Dictionary::Ptr attrs = Serialize(dobj, FAConfig);

	Dictionary::Ptr persistentItem = make_shared<Dictionary>();

	persistentItem->Set("type", GetType());
	persistentItem->Set("name", GetName());
	persistentItem->Set("properties", attrs);
	persistentItem->Set("debug_hints", debugHints.ToDictionary());

	ConfigCompilerContext::GetInstance()->WriteObject(persistentItem);

	ConfigType::Ptr ctype = ConfigType::GetByName(GetType());

	if (!ctype)
		ConfigCompilerContext::GetInstance()->AddMessage(false, "No validation type found for object '" + GetName() + "' of type '" + GetType() + "'");
	else {
		TypeRuleUtilities utils;

		try {
			attrs->Remove("name");
			ctype->ValidateItem(GetName(), attrs, GetDebugInfo(), &utils);
		} catch (const ConfigError& ex) {
			const DebugInfo *di = boost::get_error_info<errinfo_debuginfo>(ex);
			ConfigCompilerContext::GetInstance()->AddMessage(true, ex.what(), di ? *di : DebugInfo());
		} catch (const std::exception& ex) {
			ConfigCompilerContext::GetInstance()->AddMessage(true, DiagnosticInformation(ex));
		}
	}

	dobj->Register();

	m_Object = dobj;

	return dobj;
}

/**
 * Registers the configuration item.
 */
void ConfigItem::Register(void)
{
	String name = m_Name;

	/* If this is a non-abstract object we need to figure out
	 * its real name now - or assign it a temporary name. */
	if (!m_Abstract) {
		shared_ptr<NameComposer> nc = dynamic_pointer_cast<NameComposer>(Type::GetByName(m_Type));

		if (nc) {
			name = nc->MakeName(m_Name, Dictionary::Ptr());

			ASSERT(name.IsEmpty() || name == m_Name);

			if (name.IsEmpty())
				name = Utility::NewUniqueID();
		}
	}

	std::pair<String, String> key = std::make_pair(m_Type, name);
	ConfigItem::Ptr self = GetSelf();

	boost::mutex::scoped_lock lock(m_Mutex);

	m_Items[key] = self;
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
	std::pair<String, String> key = std::make_pair(type, name);
	ConfigItem::ItemMap::iterator it;

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		it = m_Items.find(key);
	}

	if (it != m_Items.end())
		return it->second;

	return ConfigItem::Ptr();
}

bool ConfigItem::ValidateItems(void)
{
	if (ConfigCompilerContext::GetInstance()->HasErrors())
		return false;

	ParallelWorkQueue upq;

	Log(LogInformation, "ConfigItem", "Committing config items");

	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		upq.Enqueue(boost::bind(&ConfigItem::Commit, kv.second, false));
	}

	upq.Join();

	std::vector<DynamicObject::Ptr> objects;
	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		DynamicObject::Ptr object = kv.second->m_Object;

		if (object)
			objects.push_back(object);
	}

	Log(LogInformation, "ConfigItem", "Triggering OnConfigLoaded signal for config items");

	BOOST_FOREACH(const DynamicObject::Ptr& object, objects) {
		upq.Enqueue(boost::bind(&DynamicObject::OnConfigLoaded, object));
	}

	upq.Join();

	Log(LogInformation, "ConfigItem", "Evaluating 'object' rules (step 1)...");
	ObjectRule::EvaluateRules(false);

	Log(LogInformation, "ConfigItem", "Evaluating 'apply' rules...");
	ApplyRule::EvaluateRules(true);

	Log(LogInformation, "ConfigItem", "Evaluating 'object' rules (step 2)...");
	ObjectRule::EvaluateRules(true);

	upq.Join();

	ConfigItem::DiscardItems();
	ConfigType::DiscardTypes();

	/* log stats for external parsers */
	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		int count = std::distance(type->GetObjects().first, type->GetObjects().second);
		if (count > 0)
			Log(LogInformation, "ConfigItem")
			    << "Checked " << count << " " << type->GetName() << "(s).";
	}

	return !ConfigCompilerContext::GetInstance()->HasErrors();
}

bool ConfigItem::ActivateItems(void)
{
	if (ConfigCompilerContext::GetInstance()->HasErrors())
		return false;

	/* restore the previous program state */
	try {
		DynamicObject::RestoreObjects(Application::GetStatePath());
	} catch (const std::exception& ex) {
		Log(LogCritical, "ConfigItem")
		    << "Failed to restore state file: " << DiagnosticInformation(ex);
	}

	Log(LogInformation, "ConfigItem", "Triggering Start signal for config items");

	ParallelWorkQueue upq;

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			if (object->IsActive())
				continue;

#ifdef _DEBUG
			Log(LogDebug, "ConfigItem")
			    << "Activating object '" << object->GetName() << "' of type '" << object->GetType()->GetName() << "'";
#endif /* _DEBUG */
			upq.Enqueue(boost::bind(&DynamicObject::Activate, object));
		}
	}

	upq.Join();

#ifdef _DEBUG
	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			ASSERT(object->IsActive());
		}
	}
#endif /* _DEBUG */

	Log(LogInformation, "ConfigItem", "Activated all objects.");

	return true;
}

void ConfigItem::DiscardItems(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	m_Items.clear();
}
