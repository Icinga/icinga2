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

ConfigItem::ItemMap ConfigItem::m_Items;
boost::signal<void (const ConfigItem::Ptr&)> ConfigItem::OnCommitted;
boost::signal<void (const ConfigItem::Ptr&)> ConfigItem::OnRemoved;

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
    const ExpressionList::Ptr& exprl, const vector<String>& parents,
    const DebugInfo& debuginfo)
	: m_Type(type), m_Name(name), m_ExpressionList(exprl),
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

/**
 * Calculates the object's properties based on parent objects and the object's
 * expression list.
 *
 * @param dictionary The dictionary that should be used to store the
 *		     properties.
 */
void ConfigItem::CalculateProperties(const Dictionary::Ptr& dictionary) const
{
	BOOST_FOREACH(const String& name, m_Parents) {
		ConfigItem::Ptr parent = ConfigItem::GetObject(GetType(), name);

		if (!parent) {
			stringstream message;
			message << "Parent object '" << name << "' does not"
			    " exist (" << m_DebugInfo << ")";
			throw_exception(domain_error(message.str()));
		}

		parent->CalculateProperties(dictionary);
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
	Logger::Write(LogDebug, "base", "Commit called for ConfigItem Type=" + GetType() + ", Name=" + GetName());

	DynamicObject::Ptr dobj = m_DynamicObject.lock();

	Dictionary::Ptr properties = boost::make_shared<Dictionary>();
	CalculateProperties(properties);

	/* Create a fake update in the format that
	 * DynamicObject::ApplyUpdate expects. */
	Dictionary::Ptr attrs = boost::make_shared<Dictionary>();

	String key;
	Value data;
	BOOST_FOREACH(tie(key, data), properties) {
		Dictionary::Ptr attr = boost::make_shared<Dictionary>();
		attr->Set("data", data);
		attr->Set("type", Attribute_Config);
		attr->Set("tx", DynamicObject::GetCurrentTx());
		attrs->Set(key, attr);
	}

	Dictionary::Ptr update = boost::make_shared<Dictionary>();
	update->Set("attrs", attrs);
	update->Set("configTx", DynamicObject::GetCurrentTx());

	DynamicType::Ptr dtype = DynamicType::GetByName(GetType());

	if (!dtype)
		throw_exception(runtime_error("Type '" + GetType() + "' does not exist."));

	if (!dobj)
		dobj = dtype->GetObject(GetName());

	if (!dobj)
		dobj = dtype->CreateObject(update);
	else
		dobj->ApplyUpdate(update, Attribute_Config);

	m_DynamicObject = dobj;

	if (dobj->IsAbstract())
		dobj->Unregister();
	else
		dobj->Register();

	pair<String, String> ikey = make_pair(GetType(), GetName());
	ItemMap::iterator it = m_Items.find(ikey);

	/* unregister the old item from its parents */
	if (it != m_Items.end()) {
		ConfigItem::Ptr oldItem = it->second;
		oldItem->UnregisterFromParents();

		/* steal the old item's children */
		m_ChildObjects = oldItem->m_ChildObjects;
	}

	/* register this item with its parents */
	BOOST_FOREACH(const String& parentName, m_Parents) {
		ConfigItem::Ptr parent = GetObject(GetType(), parentName);
		parent->RegisterChild(GetSelf());
	}

	/* We need to make a copy of the child objects becauuse the
	 * OnParentCommitted() handler is going to update the list. */
	set<ConfigItem::WeakPtr> children = m_ChildObjects;

	/* notify our children of the update */
	BOOST_FOREACH(const ConfigItem::WeakPtr wchild, children) {
		const ConfigItem::Ptr& child = wchild.lock();

		if (!child)
			continue;

		child->OnParentCommitted();
	}

	m_Items[ikey] = GetSelf();

	OnCommitted(GetSelf());

	return dobj;
}

/**
 * Unregisters the configuration item.
 */
void ConfigItem::Unregister(void)
{
	DynamicObject::Ptr dobj = m_DynamicObject.lock();

	if (dobj)
		dobj->Unregister();

	ConfigItem::ItemMap::iterator it;
	it = m_Items.find(make_pair(GetType(), GetName()));

	if (it != m_Items.end())
		m_Items.erase(it);

	UnregisterFromParents();

	OnRemoved(GetSelf());
}

void ConfigItem::RegisterChild(const ConfigItem::Ptr& child)
{
	m_ChildObjects.insert(child);
}

void ConfigItem::UnregisterChild(const ConfigItem::Ptr& child)
{
	m_ChildObjects.erase(child);
}

void ConfigItem::UnregisterFromParents(void)
{
	BOOST_FOREACH(const String& parentName, m_Parents) {
		ConfigItem::Ptr parent = GetObject(GetType(), parentName);

		if (parent)
			parent->UnregisterChild(GetSelf());
	}
}

/*
 * Notifies an item that one of its parents has been committed.
 */
void ConfigItem::OnParentCommitted(void)
{
	if (GetObject(GetType(), GetName()) == static_cast<ConfigItem::Ptr>(GetSelf()))
		Commit();
}

/**
 * Retrieves the DynamicObject that belongs to the configuration item.
 *
 * @returns The DynamicObject.
 */
DynamicObject::Ptr ConfigItem::GetDynamicObject(void) const
{
	return m_DynamicObject.lock();
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
	ConfigItem::ItemMap::iterator it;
	it = m_Items.find(make_pair(type, name));

	if (it == m_Items.end())
		return ConfigItem::Ptr();

	return it->second;
}

void ConfigItem::Dump(ostream& fp) const
{
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
