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

class I2_DYN_API ConfigItem : public Object {
public:
	typedef shared_ptr<ConfigItem> Ptr;
	typedef weak_ptr<ConfigItem> WeakPtr;

	ConfigItem(const string& type, const string& name,
	    const ExpressionList::Ptr& exprl, const vector<string>& parents,
	    const DebugInfo& debuginfo);

	string GetType(void) const;
	string GetName(void) const;

	vector<string> GetParents(void) const;

	ExpressionList::Ptr GetExpressionList(void) const;

	void CalculateProperties(Dictionary::Ptr dictionary) const;

	DynamicObject::Ptr Commit(void);
	void Unregister(void);

	DynamicObject::Ptr GetDynamicObject(void) const;

	DebugInfo GetDebugInfo(void) const;

	static ConfigItem::Ptr GetObject(const string& type, const string& name);

	static boost::signal<void (const ConfigItem::Ptr&)> OnCommitted;
	static boost::signal<void (const ConfigItem::Ptr&)> OnRemoved;

private:
	string m_Type;
	string m_Name;

	ExpressionList::Ptr m_ExpressionList;
	vector<string> m_Parents;
	DebugInfo m_DebugInfo;

	DynamicObject::WeakPtr m_DynamicObject;

	typedef map<pair<string, string>, ConfigItem::Ptr> ItemMap;
	static ItemMap m_Items;
};

}

#endif /* CONFIGITEM_H */
