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

#ifndef CONFIGITEM_H
#define CONFIGITEM_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/dynamicobject.hpp"
#include "base/workqueue.hpp"

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
	DECLARE_PTR_TYPEDEFS(ConfigItem);

	ConfigItem(const String& type, const String& name, bool abstract,
	    const boost::shared_ptr<Expression>& exprl,
	    const boost::shared_ptr<Expression>& filter,
	    const DebugInfo& debuginfo,
	    const Dictionary::Ptr& scope, const String& zone);

	String GetType(void) const;
	String GetName(void) const;
	bool IsAbstract(void) const;

	std::vector<ConfigItem::Ptr> GetParents(void) const;

	boost::shared_ptr<Expression> GetExpression(void) const;
	boost::shared_ptr<Expression> GetFilter(void) const;

	DynamicObject::Ptr Commit(bool discard = true);
	void Register(void);

	DebugInfo GetDebugInfo(void) const;
	Dictionary::Ptr GetScope(void) const;

	String GetZone(void) const;

	static ConfigItem::Ptr GetObject(const String& type,
	    const String& name);

	static bool CommitItems(void);
	static bool ActivateItems(void);

	static std::vector<ConfigItem::Ptr> GetItems(const String& type);

private:
	String m_Type; /**< The object type. */
	String m_Name; /**< The name. */
	bool m_Abstract; /**< Whether this is a template. */

	boost::shared_ptr<Expression> m_Expression;
	boost::shared_ptr<Expression> m_Filter;
	DebugInfo m_DebugInfo; /**< Debug information. */
	Dictionary::Ptr m_Scope; /**< variable scope. */
	String m_Zone; /**< The zone. */

	DynamicObject::Ptr m_Object;

	static boost::mutex m_Mutex;

	typedef std::map<String, ConfigItem::Ptr> ItemMap;
	typedef std::map<String, ItemMap> TypeMap;
	static TypeMap m_Items; /**< All registered configuration items. */

	typedef std::vector<ConfigItem::Ptr> ItemList;
	static ItemList m_UnnamedItems;
	static ItemList m_CommittedItems;

	static ConfigItem::Ptr GetObjectUnlocked(const String& type,
	    const String& name);

	static bool CommitNewItems(WorkQueue& upq);
};

}

#endif /* CONFIGITEM_H */
