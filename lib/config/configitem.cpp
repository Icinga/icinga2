/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-config.h"

using namespace icinga;

recursive_mutex ConfigItem::m_Mutex;
ConfigItem::ItemMap ConfigItem::m_Items;
signals2::signal<void (const ConfigItem::Ptr&)> ConfigItem::OnCommitted;
signals2::signal<void (const ConfigItem::Ptr&)> ConfigItem::OnRemoved;

/**
 * Constructor for the ConfigItem class.
 *
 * @param type The object type.
 * @param name The name of the item.
 * @param exprl Expression list for the item.
 * @param parents Parent objects for the item.
 * @param debuginfo Debug information.
 */
ConfigItem::ConfigItem(const String& type, const String& name,
    const String& unit, const ExpressionList::Ptr& exprl,
    const vector<String>& parents, const DebugInfo& debuginfo)
	: m_Type(type), m_Name(name), m_Unit(unit), m_ExpressionList(exprl),
	  m_Parents(parents), m_DebugInfo(debuginfo)
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
 * Retrieves the name of the compilation unit this item belongs to.
 *
 * @returns The unit name.
 */
String ConfigItem::GetUnit(void) const
{
	return m_Unit;
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

/**
 * Retrieves the list of parents for the configuration item.
 *
 * @returns The list of parents.
 */
vector<String> ConfigItem::GetParents(void) const
{
	return m_Parents;
}

Dictionary::Ptr ConfigItem::Link(void) const
{
	Dictionary::Ptr attrs = boost::make_shared<Dictionary>();
	InternalLink(attrs);
	return attrs;
}

/**
 * Calculates the object's properties based on parent objects and the object's
 * expression list.
 *
 * @param dictionary The dictionary that should be used to store the
 *		     properties.
 */
void ConfigItem::InternalLink(const Dictionary::Ptr& dictionary) const
{
	ObjectLock olock(this);

	BOOST_FOREACH(const String& name, m_Parents) {
		ConfigItem::Ptr parent;

		ConfigCompilerContext *context = ConfigCompilerContext::GetContext();

		if (context)
			parent = context->GetItem(m_Type, name);

		/* ignore already active objects while we're in the compiler
		 * context and linking to existing items is disabled. */
		if (!parent && (!context || (context->GetFlags() & CompilerLinkExisting)))
			parent = ConfigItem::GetObject(m_Type, name);

		if (!parent) {
			stringstream message;
			message << "Parent object '" << name << "' does not"
			    " exist (" << m_DebugInfo << ")";
			BOOST_THROW_EXCEPTION(domain_error(message.str()));
		}

		parent->InternalLink(dictionary);
	}

	m_ExpressionList->Execute(dictionary);
}

/**
 * Commits the configuration item by creating or updating a DynamicObject
 * object.
 *
 * @returns The DynamicObject that was created/updated.
 */
DynamicObject::Ptr ConfigItem::Commit(void)
{
	assert(!OwnsLock());

	String type, name;

	Logger::Write(LogDebug, "base", "Commit called for ConfigItem Type=" + GetType() + ", Name=" + GetName());

	/* Make sure the type is valid. */
	DynamicType::Ptr dtype = DynamicType::GetByName(GetType());

	if (!dtype)
		BOOST_THROW_EXCEPTION(runtime_error("Type '" + GetType() + "' does not exist."));

	/* Try to find an existing item with the same type and name. */
	pair<String, String> ikey = make_pair(GetType(), GetName());
	ConfigItem::Ptr oldItem;

	{
		recursive_mutex::scoped_lock lock(m_Mutex);

		ItemMap::iterator it = m_Items.find(ikey);

		if (it != m_Items.end())
			oldItem = it->second;
	}

	set<ConfigItem::WeakPtr> children;

	if (oldItem) {
		ObjectLock olock(oldItem);

		/* Unregister the old item from its parents. */
		oldItem->UnregisterFromParents();

		/* Steal the old item's children. */
		children = oldItem->m_ChildObjects;
	}

	{
		ObjectLock olock(this);
		m_ChildObjects = children;
	}

	ConfigItem::Ptr self = GetSelf();

	{
		recursive_mutex::scoped_lock lock(m_Mutex);

		/* Register this item. */
		m_Items[ikey] = self;
	}

	DynamicObject::Ptr dobj = m_DynamicObject.lock();

	if (!dobj)
		dobj = dtype->GetObject(m_Name);

	/* Register this item with its parents. */
	BOOST_FOREACH(const String& parentName, m_Parents) {
		ConfigItem::Ptr parent = GetObject(m_Type, parentName);
		parent->m_ChildObjects.insert(self);
	}

	/* Create a fake update in the format that
	 * DynamicObject::ApplyUpdate expects. */
	Dictionary::Ptr attrs = boost::make_shared<Dictionary>();

	double tx = DynamicObject::GetCurrentTx();

	Dictionary::Ptr properties = Link();

	{
		ObjectLock olock(properties);

		String key;
		Value data;
		BOOST_FOREACH(tie(key, data), properties) {
			Dictionary::Ptr attr = boost::make_shared<Dictionary>();
			attr->Set("data", data);
			attr->Set("type", Attribute_Config);
			attr->Set("tx", tx);
			attr->Seal();

			attrs->Set(key, attr);
		}
	}

	attrs->Seal();

	Dictionary::Ptr update = boost::make_shared<Dictionary>();
	update->Set("attrs", attrs);
	update->Set("configTx", DynamicObject::GetCurrentTx());
	update->Seal();

	/* Update or create the object and apply the configuration settings. */
	bool was_null = false;

	if (!dobj) {
		dobj = dtype->CreateObject(update);
		was_null = true;
	}

	if (!was_null)
		dobj->ApplyUpdate(update, Attribute_Config);

	{
		ObjectLock olock(this);

		m_DynamicObject = dobj;
	}

	if (dobj->IsAbstract())
		dobj->Unregister();
	else
		dobj->Register();

	/* notify our children of the update */
	BOOST_FOREACH(const ConfigItem::WeakPtr wchild, children) {
		const ConfigItem::Ptr& child = wchild.lock();

		if (!child)
			continue;

		child->OnParentCommitted();
	}

	OnCommitted(self);

	return dobj;
}

/**
 * Unregisters the configuration item.
 */
void ConfigItem::Unregister(void)
{
	assert(!OwnsLock());

	DynamicObject::Ptr dobj = m_DynamicObject.lock();

	if (dobj)
		dobj->Unregister();

	{
		ObjectLock olock(this);

		ConfigItem::ItemMap::iterator it;
		it = m_Items.find(make_pair(m_Type, m_Name));

		if (it != m_Items.end())
			m_Items.erase(it);

		UnregisterFromParents();
	}

	OnRemoved(GetSelf());
}

void ConfigItem::UnregisterFromParents(void)
{
	assert(OwnsLock());

	BOOST_FOREACH(const String& parentName, m_Parents) {
		ConfigItem::Ptr parent = GetObject(GetType(), parentName);

		if (parent)
			parent->m_ChildObjects.erase(GetSelf());
	}
}

/*
 * Notifies an item that one of its parents has been committed.
 */
void ConfigItem::OnParentCommitted(void)
{
	assert(!OwnsLock());

	ConfigItem::Ptr self = GetSelf();

	if (GetObject(m_Type, m_Name) != self)
		return;

	Commit();
}

/**
 * Retrieves the DynamicObject that belongs to the configuration item.
 *
 * @returns The DynamicObject.
 */
DynamicObject::Ptr ConfigItem::GetDynamicObject(void) const
{
	ObjectLock olock(this);

	return m_DynamicObject.lock();
}

/**
 * Retrieves a configuration item by type and name.
 *
 * @param type The type of the ConfigItem that is to be looked up.
 * @param name The name of the ConfigItem that is to be looked up.
 * @returns The configuration item.
 * @threadsafety Always.
 */
ConfigItem::Ptr ConfigItem::GetObject(const String& type, const String& name)
{
	recursive_mutex::scoped_lock lock(m_Mutex);

	ConfigItem::ItemMap::iterator it;

	it = m_Items.find(make_pair(type, name));

	if (it != m_Items.end())
		return it->second;

	return ConfigItem::Ptr();
}

/**
 * Dumps the config item to the specified stream using Icinga's config item
 * syntax.
 *
 * @param fp The stream.
 */
void ConfigItem::Dump(ostream& fp) const
{
	ObjectLock olock(this);

	fp << "object \"" << m_Type << "\" \"" << m_Name << "\"";

	if (m_Parents.size() > 0) {
		fp << " inherits";

		bool first = true;
		BOOST_FOREACH(const String& name, m_Parents) {
			if (!first)
				fp << ",";
			else
				first = false;

			fp << " \"" << name << "\"";
		}
	}

	fp << " {" << "\n";
	m_ExpressionList->Dump(fp, 1);
	fp << "}" << "\n";
}

/**
 * @threadsafety Always.
 */
void ConfigItem::UnloadUnit(const String& unit)
{
	recursive_mutex::scoped_lock lock(m_Mutex);

	Logger::Write(LogInformation, "config", "Unloading config items from compilation unit '" + unit + "'");

	vector<ConfigItem::Ptr> obsoleteItems;

	ConfigItem::Ptr item;
	BOOST_FOREACH(tie(tuples::ignore, item), m_Items) {
		ObjectLock olock(item);

		if (item->m_Unit != unit)
			continue;

		obsoleteItems.push_back(item);
	}

	BOOST_FOREACH(const ConfigItem::Ptr& item, obsoleteItems) {
		item->Unregister();
	}
}
