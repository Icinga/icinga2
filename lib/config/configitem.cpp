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

ConfigItem::ConfigItem(const String& type, const String& name,
    const ExpressionList::Ptr& exprl, const vector<String>& parents,
    const DebugInfo& debuginfo)
	: m_Type(type), m_Name(name), m_ExpressionList(exprl),
	  m_Parents(parents), m_DebugInfo(debuginfo)
{
}

String ConfigItem::GetType(void) const
{
	return m_Type;
}

String ConfigItem::GetName(void) const
{
	return m_Name;
}

DebugInfo ConfigItem::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

ExpressionList::Ptr ConfigItem::GetExpressionList(void) const
{
	return m_ExpressionList;
}

vector<String> ConfigItem::GetParents(void) const
{
	return m_Parents;
}

void ConfigItem::CalculateProperties(const Dictionary::Ptr& dictionary) const
{
	BOOST_FOREACH(const String& name, m_Parents) {
		ConfigItem::Ptr parent = ConfigItem::GetObject(GetType(), name);

		if (!parent) {
			stringstream message;
			message << "Parent object '" << name << "' does not exist (" << m_DebugInfo << ")";
			throw_exception(domain_error(message.str()));
		}

		parent->CalculateProperties(dictionary);
	}

	m_ExpressionList->Execute(dictionary);
}

DynamicObject::Ptr ConfigItem::Commit(void)
{
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

	if (!dobj)
		dobj = DynamicObject::GetObject(GetType(), GetName());

	if (!dobj)
		dobj = DynamicObject::Create(GetType(), update);
	else
		dobj->ApplyUpdate(update, Attribute_Config);

	m_DynamicObject = dobj;

	if (dobj->IsAbstract())
		dobj->Unregister();
	else
		dobj->Register();

	/* TODO: Figure out whether there are any child objects which inherit
	 * from this config item and Commit() them as well */

	m_Items[make_pair(GetType(), GetName())] = GetSelf();

	OnCommitted(GetSelf());

	return dobj;
}

void ConfigItem::Unregister(void)
{
	DynamicObject::Ptr dobj = m_DynamicObject.lock();

	if (dobj)
		dobj->Unregister();

	ConfigItem::ItemMap::iterator it;
	it = m_Items.find(make_pair(GetType(), GetName()));

	if (it != m_Items.end())
		m_Items.erase(it);

	OnRemoved(GetSelf());
}

DynamicObject::Ptr ConfigItem::GetDynamicObject(void) const
{
	return m_DynamicObject.lock();
}

ConfigItem::Ptr ConfigItem::GetObject(const String& type, const String& name)
{
	ConfigItem::ItemMap::iterator it;
	it = m_Items.find(make_pair(type, name));

	if (it == m_Items.end())
		return ConfigItem::Ptr();

	return it->second;
}
