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
#include "base/logger_fwd.hpp"
#include "base/debug.hpp"
#include "base/workqueue.hpp"
#include "base/exception.hpp"
#include "base/stdiostream.hpp"
#include "base/netstring.hpp"
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
    bool abstract, const AExpression::Ptr& exprl,
    const DebugInfo& debuginfo, const Dictionary::Ptr& scope,
    const String& zone)
	: m_Type(type), m_Name(name), m_Abstract(abstract), m_Validated(false),
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

Dictionary::Ptr ConfigItem::GetScope(void) const
{
	return m_Scope;
}

/**
 * Retrieves the expression list for the configuration item.
 *
 * @returns The expression list.
 */
AExpression::Ptr ConfigItem::GetExpressionList(void) const
{
	return m_ExpressionList;
}

Dictionary::Ptr ConfigItem::GetProperties(void)
{
	ASSERT(OwnsLock());

	if (!m_Properties) {
		DebugHint dhint;
		m_Properties = make_shared<Dictionary>();
		m_Properties->Set("type", m_Type);
		if (!m_Zone.IsEmpty())
			m_Properties->Set("zone", m_Zone);
		m_Properties->Set("__parent", m_Scope);
		GetExpressionList()->Evaluate(m_Properties, &dhint);
		m_Properties->Remove("__parent");
		m_DebugHints = dhint.ToDictionary();

		String name = m_Name;

		if (!m_Abstract) {
			const NameComposer *nc = dynamic_cast<const NameComposer *>(Type::GetByName(m_Type));

			if (nc) {
				name = nc->MakeName(m_Name, m_Properties);

				if (name.IsEmpty())
					BOOST_THROW_EXCEPTION(std::runtime_error("Could not determine name for object"));
			}
		}

		if (name != m_Name)
			m_Properties->Set("name", m_Name);

		m_Properties->Set("__name", name);

		VERIFY(m_Properties->Get("type") == GetType());
	}

	return m_Properties;
}

Dictionary::Ptr ConfigItem::GetDebugHints(void) const
{
	return m_DebugHints;
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
	Log(LogDebug, "ConfigItem", "Commit called for ConfigItem Type=" + GetType() + ", Name=" + GetName());
#endif /* _DEBUG */

	/* Make sure the type is valid. */
	DynamicType::Ptr dtype = DynamicType::GetByName(GetType());

	if (!dtype)
		BOOST_THROW_EXCEPTION(std::runtime_error("Type '" + GetType() + "' does not exist."));

	if (dtype->GetObject(GetName()))
	    BOOST_THROW_EXCEPTION(std::runtime_error("An object with type '" + GetType() + "' and name '" + GetName() + "' already exists."));

	if (IsAbstract())
		return DynamicObject::Ptr();

	Dictionary::Ptr properties;

	{
		ObjectLock olock(this);

		properties = GetProperties();
	}

	DynamicObject::Ptr dobj = dtype->CreateObject(properties);
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
		const NameComposer *nc = dynamic_cast<const NameComposer *>(Type::GetByName(m_Type));

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

void ConfigItem::WriteObjectsFile(const String& filename)
{
	Log(LogInformation, "ConfigItem", "Dumping config items to file '" + filename + "'");

	String tempFilename = filename + ".tmp";

	std::fstream fp;
	fp.open(tempFilename.CStr(), std::ios_base::out);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + tempFilename + "' file"));

	StdioStream::Ptr sfp = make_shared<StdioStream>(&fp, false);

	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		ConfigItem::Ptr item = kv.second;

		Dictionary::Ptr persistentItem = make_shared<Dictionary>();

		persistentItem->Set("type", item->GetType());
		persistentItem->Set("name", item->GetName());
		persistentItem->Set("abstract", item->IsAbstract());
		persistentItem->Set("properties", item->GetProperties());
		persistentItem->Set("debug_hints", item->GetDebugHints());

		String json = JsonSerialize(persistentItem);

		NetString::WriteStringToStream(sfp, json);
	}

	sfp->Close();

	fp.close();

#ifdef _WIN32
	_unlink(filename.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), filename.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempFilename));
	}
}

bool ConfigItem::ValidateItems(const String& objectsFile)
{
	if (ConfigCompilerContext::GetInstance()->HasErrors())
		return false;

	ParallelWorkQueue upq;

	Log(LogInformation, "ConfigItem", "Validating config items (step 1)...");

	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		upq.Enqueue(boost::bind(&ConfigItem::ValidateItem, kv.second));
	}

	upq.Join();

	if (ConfigCompilerContext::GetInstance()->HasErrors())
		return false;

	Log(LogInformation, "ConfigItem", "Committing config items");

	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		upq.Enqueue(boost::bind(&ConfigItem::Commit, kv.second));
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

	Log(LogInformation, "ConfigItem", "Validating config items (step 2)...");

	BOOST_FOREACH(const ItemMap::value_type& kv, m_Items) {
		upq.Enqueue(boost::bind(&ConfigItem::ValidateItem, kv.second));
	}

	upq.Join();

	if (!objectsFile.IsEmpty())
		ConfigItem::WriteObjectsFile(objectsFile);

	ConfigItem::DiscardItems();
	ConfigType::DiscardTypes();

	/* log stats for external parsers */
	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		int count = std::distance(type->GetObjects().first, type->GetObjects().second);
		if (count > 0)
			Log(LogInformation, "ConfigItem", "Checked " + Convert::ToString(count) + " " + type->GetName() + "(s).");
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
		Log(LogCritical, "ConfigItem", "Failed to restore state file: " + DiagnosticInformation(ex));
	}

	Log(LogInformation, "ConfigItem", "Triggering Start signal for config items");

	ParallelWorkQueue upq;

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			if (object->IsActive())
				continue;

#ifdef _DEBUG
			Log(LogDebug, "ConfigItem", "Activating object '" + object->GetName() + "' of type '" + object->GetType()->GetName() + "'");
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
