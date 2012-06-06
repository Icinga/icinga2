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

#ifndef CONFIGITEM_H
#define CONFIGITEM_H

namespace icinga
{

class ConfigItem : public Object {
public:
	typedef shared_ptr<ConfigItem> Ptr;
	typedef weak_ptr<ConfigItem> WeakPtr;

	typedef ObjectMap<pair<string, string>, ConfigItem::Ptr> TNMap;

	ConfigItem(string type, string name, DebugInfo debuginfo);

	string GetType(void) const;
	string GetName(void) const;

	vector<string> GetParents(void) const;
	void AddParent(string parent);

	ExpressionList::Ptr GetExpressionList(void) const;
	void SetExpressionList(const ExpressionList::Ptr& exprl);

	Dictionary::Ptr CalculateProperties(void) const;

	void Commit(void);
	void Unregister(void);

	static ObjectSet<ConfigItem::Ptr>::Ptr GetAllObjects(void);
	static TNMap::Ptr GetObjectsByTypeAndName(void);
	static ConfigItem::Ptr GetObject(string type, string name);

private:
	string m_Type;
	string m_Name;
	DebugInfo m_DebugInfo;
	vector<string> m_Parents;
	ExpressionList::Ptr m_ExpressionList;

	DynamicObject::WeakPtr m_DynamicObject;

	static bool GetTypeAndName(const ConfigItem::Ptr& object, pair<string, string> *key);
};


}

#endif /* CONFIGITEM_H */
