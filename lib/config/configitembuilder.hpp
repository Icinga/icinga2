/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef CONFIGITEMBUILDER_H
#define CONFIGITEMBUILDER_H

#include "config/expression.hpp"
#include "config/configitem.hpp"
#include "base/debuginfo.hpp"
#include "base/object.hpp"

namespace icinga
{

/**
 * Config item builder. Used to dynamically build configuration objects
 * at runtime.
 *
 * @ingroup config
 */
class I2_CONFIG_API ConfigItemBuilder : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigItemBuilder);

	ConfigItemBuilder(void);
	explicit ConfigItemBuilder(const DebugInfo& debugInfo);

	void SetType(const String& type);
	void SetName(const String& name);
	void SetAbstract(bool abstract);
	void SetScope(const Dictionary::Ptr& scope);
	void SetZone(const String& zone);
	void SetPackage(const String& package);
	void SetIgnoreOnError(bool ignoreOnError);

	void AddExpression(Expression *expr);
	void SetFilter(const boost::shared_ptr<Expression>& filter);

	ConfigItem::Ptr Compile(void);

private:
	String m_Type; /**< The object type. */
	String m_Name; /**< The name. */
	bool m_Abstract; /**< Whether the item is abstract. */
	std::vector<Expression *> m_Expressions; /**< Expressions for this item. */
	boost::shared_ptr<Expression> m_Filter; /**< Filter expression. */
	DebugInfo m_DebugInfo; /**< Debug information. */
	Dictionary::Ptr m_Scope; /**< variable scope. */
	String m_Zone; /**< The zone. */
	String m_Package; /**< The package name. */
	bool m_IgnoreOnError; /**< Whether the object should be ignored when an error occurs in one of the expressions. */
};

}

#endif /* CONFIGITEMBUILDER */
