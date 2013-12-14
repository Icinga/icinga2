/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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
	DECLARE_PTR_TYPEDEFS(ConfigItem);

	ConfigItem(const String& type, const String& name, bool abstract,
	    const ExpressionList::Ptr& exprl, const std::vector<String>& parents,
	    const DebugInfo& debuginfo);

	String GetType(void) const;
	String GetName(void) const;
	bool IsAbstract(void) const;

	std::vector<ConfigItem::Ptr> GetParents(void) const;

	ExpressionList::Ptr GetLinkedExpressionList(void);
	Dictionary::Ptr GetProperties(void);

	DynamicObject::Ptr Commit(void);
	void Register(void);

	DebugInfo GetDebugInfo(void) const;

	static ConfigItem::Ptr GetObject(const String& type,
	    const String& name);
	static bool HasObject(const String& type, const String& name);

	void ValidateItem(void);
        
	static bool ActivateItems(bool validateOnly);
	static void DiscardItems(void);

private:
	ExpressionList::Ptr GetExpressionList(void) const;

	String m_Type; /**< The object type. */
	String m_Name; /**< The name. */
	bool m_Abstract; /**< Whether this is a template. */
	bool m_Validated; /** Whether this object has been validated. */

	ExpressionList::Ptr m_ExpressionList;
	std::vector<String> m_ParentNames; /**< The names of parent configuration
				       items. */
	DebugInfo m_DebugInfo; /**< Debug information. */

	ExpressionList::Ptr m_LinkedExpressionList;
	Dictionary::Ptr m_Properties;
        
        DynamicObject::Ptr m_Object;

	static boost::mutex m_Mutex;

	typedef std::map<std::pair<String, String>, ConfigItem::Ptr> ItemMap;
	static ItemMap m_Items; /**< All registered configuration items. */

	static ConfigItem::Ptr GetObjectUnlocked(const String& type,
	    const String& name);
};

}

#endif /* CONFIGITEM_H */
