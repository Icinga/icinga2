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

#include "i2-dyn.h"

using namespace icinga;

ConfigItem::ItemMap ConfigItem::m_Items;
boost::signal<void (const ConfigItem::Ptr&)> ConfigItem::OnCommitted;
boost::signal<void (const ConfigItem::Ptr&)> ConfigItem::OnRemoved;

ConfigItem::ConfigItem(const string& type, const string& name,
    const ExpressionList::Ptr& exprl, const vector<string>& parents,
    const DebugInfo& debuginfo)
	: m_Type(type), m_Name(name), m_ExpressionList(exprl),
	  m_Parents(parents), m_DebugInfo(debuginfo)
{
}

string ConfigItem::GetType(void) const
{
	return m_Type;
}

string ConfigItem::GetName(void) const
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

vector<string> ConfigItem::GetParents(void) const
{
	return m_Parents;
}

void ConfigItem::CalculateProperties(Dictionary::Ptr dictionary) const
{
	BOOST_FOREACH(const string& name, m_Parents) {
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

ConfigObject::Ptr ConfigItem::Commit(void)
{
	ConfigObject::Ptr dobj = m_ConfigObject.lock();

	Dictionary::Ptr properties = boost::make_shared<Dictionary>();
	CalculateProperties(properties);

	if (!dobj)
		dobj = ConfigObject::GetObject(GetType(), GetName());

	if (!dobj)
		dobj = ConfigObject::Create(GetType(), properties);
	else
		dobj->SetProperties(properties);

	m_ConfigObject = dobj;

	if (dobj->IsAbstract())
		dobj->Unregister();
	else
		dobj->Commit();

	/* TODO: Figure out whether there are any child objects which inherit
	 * from this config item and Commit() them as well */

	m_Items[make_pair(GetType(), GetName())] = GetSelf();

	OnCommitted(GetSelf());

	return dobj;
}

void ConfigItem::Unregister(void)
{
	ConfigObject::Ptr dobj = m_ConfigObject.lock();

	if (dobj)
		dobj->Unregister();

	ConfigItem::ItemMap::iterator it;
	it = m_Items.find(make_pair(GetType(), GetName()));

	if (it != m_Items.end())
		m_Items.erase(it);

	OnRemoved(GetSelf());
}

ConfigObject::Ptr ConfigItem::GetConfigObject(void) const
{
	return m_ConfigObject.lock();
}

ConfigItem::Ptr ConfigItem::GetObject(const string& type, const string& name)
{
	ConfigItem::ItemMap::iterator it;
	it = m_Items.find(make_pair(type, name));

	if (it == m_Items.end())
		return ConfigItem::Ptr();

	return it->second;
}
