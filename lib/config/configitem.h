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

#include "config/i2-config.h"
#include "config/expressionlist.h"
#include "base/dynamicobject.h"

namespace icinga
{

/**
 * A configuration item. Non-abstract configuration items can be used to
 * create configuration objects at runtime.
 *
 * @ingroup config
 */
class I2_CONFIG_API ConfigItem : public Object {
public:
	typedef shared_ptr<ConfigItem> Ptr;
	typedef weak_ptr<ConfigItem> WeakPtr;

	ConfigItem(const String& type, const String& name, const String& unit,
	    bool abstract, const ExpressionList::Ptr& exprl, const std::vector<String>& parents,
	    const DebugInfo& debuginfo);

	String GetType(void) const;
	String GetName(void) const;
	String GetUnit(void) const;
	bool IsAbstract(void) const;

	std::vector<ConfigItem::Ptr> GetParents(void) const;

	void Link(void);
	ExpressionList::Ptr GetLinkedExpressionList(void) const;

	void GetProperties(void);

	DynamicObject::Ptr Commit(void);
	void Register(void);
	void Unregister(void);

	void Dump(std::ostream& fp) const;

	DynamicObject::Ptr GetDynamicObject(void) const;

	DebugInfo GetDebugInfo(void) const;

	static ConfigItem::Ptr GetObject(const String& type,
	    const String& name);

	static void UnloadUnit(const String& unit);

	static boost::signals2::signal<void (const ConfigItem::Ptr&)> OnCommitted;
	static boost::signals2::signal<void (const ConfigItem::Ptr&)> OnRemoved;

private:
	ExpressionList::Ptr GetExpressionList(void) const;

	void UnregisterFromParents(void);

	void OnParentCommitted(void);

	String m_Type; /**< The object type. */
	String m_Name; /**< The name. */
	String m_Unit; /**< The compilation unit. */
	bool m_Abstract; /**< Whether this is a template. */

	ExpressionList::Ptr m_ExpressionList;
	std::vector<String> m_ParentNames; /**< The names of parent configuration
				       items. */
	std::vector<ConfigItem::Ptr> m_Parents;
	DebugInfo m_DebugInfo; /**< Debug information. */

	ExpressionList::Ptr m_LinkedExpressionList;

	DynamicObject::WeakPtr m_DynamicObject; /**< The instantiated version
                                                 * of this configuration item */
	std::set<ConfigItem::WeakPtr> m_ChildObjects; /**< Instantiated items
                                                     * that inherit from this item */

	static boost::mutex m_Mutex;

	typedef std::map<std::pair<String, String>, ConfigItem::Ptr, pair_string_iless> ItemMap;
	static ItemMap m_Items; /**< All registered configuration items. */

	static ConfigItem::Ptr GetObjectUnlocked(const String& type,
	    const String& name);
};

}

#endif /* CONFIGITEM_H */
