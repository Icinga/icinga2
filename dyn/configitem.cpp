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

ConfigItem::ConfigItem(const string& type, const string& name, const DebugInfo& debuginfo)
	: m_Type(type), m_Name(name), m_DebugInfo(debuginfo)
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

void ConfigItem::SetExpressionList(const ExpressionList::Ptr& exprl)
{
	m_ExpressionList = exprl;
}

vector<string> ConfigItem::GetParents(void) const
{
	return m_Parents;
}

void ConfigItem::AddParent(const string& parent)
{
	m_Parents.push_back(parent);
}

void ConfigItem::CalculateProperties(Dictionary::Ptr dictionary) const
{
	vector<string>::const_iterator it;
	for (it = m_Parents.begin(); it != m_Parents.end(); it++) {
		ConfigItem::Ptr parent = ConfigItem::GetObject(GetType(), *it);

		if (!parent) {
			stringstream message;
			message << "Parent object '" << *it << "' does not exist (" << m_DebugInfo << ")";
			throw domain_error(message.str());
		}

		parent->CalculateProperties(dictionary);
	}

	m_ExpressionList->Execute(dictionary);
}

ConfigItem::Set::Ptr ConfigItem::GetAllObjects(void)
{
	static ObjectSet<ConfigItem::Ptr>::Ptr allObjects;

        if (!allObjects) {
                allObjects = boost::make_shared<ObjectSet<ConfigItem::Ptr> >();
                allObjects->Start();
        }

        return allObjects;
}

bool ConfigItem::GetTypeAndName(const ConfigItem::Ptr& object, pair<string, string> *key)
{
	*key = make_pair(object->GetType(), object->GetName());

	return true;
}

ConfigItem::TNMap::Ptr ConfigItem::GetObjectsByTypeAndName(void)
{
	static ConfigItem::TNMap::Ptr tnmap;

	if (!tnmap) {
		tnmap = boost::make_shared<ConfigItem::TNMap>(GetAllObjects(), &ConfigItem::GetTypeAndName);
		tnmap->Start();
	}

	return tnmap;
}

ConfigObject::Ptr ConfigItem::Commit(void)
{
	ConfigObject::Ptr dobj = m_ConfigObject.lock();

	Dictionary::Ptr properties = boost::make_shared<Dictionary>();
	CalculateProperties(properties);

	if (!dobj)
		dobj = ConfigObject::GetObject(GetType(), GetName());

	if (!dobj)
		dobj = boost::make_shared<ConfigObject>(properties);
	else
		dobj->SetProperties(properties);

	m_ConfigObject = dobj;

	if (dobj->IsAbstract())
		dobj->Unregister();
	else
		dobj->Commit();

	/* TODO: Figure out whether there are any child objects which inherit
	 * from this config item and Commit() them as well */

	ConfigItem::Ptr ci = GetObject(GetType(), GetName());
	ConfigItem::Ptr self = GetSelf();
	if (ci && ci != self) {
		ci->m_ConfigObject.reset();
		GetAllObjects()->RemoveObject(ci);
	}
	GetAllObjects()->CheckObject(self);

	return dobj;
}

void ConfigItem::Unregister(void)
{
	ConfigObject::Ptr dobj = m_ConfigObject.lock();

	if (dobj)
		dobj->Unregister();

	GetAllObjects()->RemoveObject(GetSelf());
}

ConfigObject::Ptr ConfigItem::GetConfigObject(void) const
{
	return m_ConfigObject.lock();
}

ConfigItem::Ptr ConfigItem::GetObject(const string& type, const string& name)
{
	ConfigItem::TNMap::Range range;
	range = GetObjectsByTypeAndName()->GetRange(make_pair(type, name));

	assert(distance(range.first, range.second) <= 1);

	if (range.first == range.second)
		return ConfigItem::Ptr();
	else
		return range.first->second;
}
