/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "config/activationcontext.hpp"
#include "base/configobject.hpp"
#include "base/workqueue.hpp"

namespace icinga
{


/**
 * A configuration item. Non-abstract configuration items can be used to
 * create configuration objects at runtime.
 *
 * @ingroup config
 */
class ConfigItem final : public Object {
public:
	DECLARE_PTR_TYPEDEFS(ConfigItem);

	ConfigItem(const Type::Ptr& type, const String& name, bool abstract,
		const std::shared_ptr<Expression>& exprl,
		const std::shared_ptr<Expression>& filter,
		bool defaultTmpl, bool ignoreOnError, const DebugInfo& debuginfo,
		const Dictionary::Ptr& scope, const String& zone,
		const String& package);

	Type::Ptr GetType() const;
	String GetName() const;
	bool IsAbstract() const;
	bool IsDefaultTemplate() const;
	bool IsIgnoreOnError() const;

	std::vector<ConfigItem::Ptr> GetParents() const;

	std::shared_ptr<Expression> GetExpression() const;
	std::shared_ptr<Expression> GetFilter() const;

	void Register();
	void Unregister();

	DebugInfo GetDebugInfo() const;
	Dictionary::Ptr GetScope() const;

	ConfigObject::Ptr GetObject() const;

	static ConfigItem::Ptr GetByTypeAndName(const Type::Ptr& type,
		const String& name);

	static bool CommitItems(const ActivationContext::Ptr& context, WorkQueue& upq, std::vector<ConfigItem::Ptr>& newItems, bool silent = false);
	static bool ActivateItems(WorkQueue& upq, const std::vector<ConfigItem::Ptr>& newItems, bool runtimeCreated = false, bool silent = false, bool withModAttrs = false);

	static bool RunWithActivationContext(const Function::Ptr& function);

	static std::vector<ConfigItem::Ptr> GetItems(const Type::Ptr& type);
	static std::vector<ConfigItem::Ptr> GetDefaultTemplates(const Type::Ptr& type);

	static void RemoveIgnoredItems(const String& allowedConfigPath);

private:
	Type::Ptr m_Type; /**< The object type. */
	String m_Name; /**< The name. */
	bool m_Abstract; /**< Whether this is a template. */

	std::shared_ptr<Expression> m_Expression;
	std::shared_ptr<Expression> m_Filter;
	bool m_DefaultTmpl;
	bool m_IgnoreOnError;
	DebugInfo m_DebugInfo; /**< Debug information. */
	Dictionary::Ptr m_Scope; /**< variable scope. */
	String m_Zone; /**< The zone. */
	String m_Package;
	ActivationContext::Ptr m_ActivationContext;

	ConfigObject::Ptr m_Object;

	static boost::mutex m_Mutex;

	typedef std::map<String, ConfigItem::Ptr> ItemMap;
	typedef std::map<Type::Ptr, ItemMap> TypeMap;
	static TypeMap m_Items; /**< All registered configuration items. */
	static TypeMap m_DefaultTemplates;

	typedef std::vector<ConfigItem::Ptr> ItemList;
	static ItemList m_UnnamedItems;

	typedef std::vector<String> IgnoredItemList;
	static IgnoredItemList m_IgnoredItems;

	static ConfigItem::Ptr GetObjectUnlocked(const String& type,
		const String& name);

	ConfigObject::Ptr Commit(bool discard = true);

	static bool CommitNewItems(const ActivationContext::Ptr& context, WorkQueue& upq, std::vector<ConfigItem::Ptr>& newItems);
};

}

#endif /* CONFIGITEM_H */
