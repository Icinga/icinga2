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

#ifndef CONFIGITEMBUILDER_H
#define CONFIGITEMBUILDER_H

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
	typedef shared_ptr<ConfigItemBuilder> Ptr;
	typedef weak_ptr<ConfigItemBuilder> WeakPtr;

	ConfigItemBuilder(void);
	explicit ConfigItemBuilder(const DebugInfo& debugInfo);

	void SetType(const String& type);
	void SetName(const String& name);
	void SetUnit(const String& unit);
	void SetLocal(bool local);
	void SetAbstract(bool abstract);

	void AddParent(const String& parent);

	void AddExpression(const Expression& expr);
	void AddExpression(const String& key, ExpressionOperator op,
	    const Value& value);
	void AddExpressionList(const ExpressionList::Ptr& exprl);

	ConfigItem::Ptr Compile(void);

private:
	String m_Type; /**< The object type. */
	String m_Name; /**< The name. */
	String m_Unit; /**< The compilation unit. */
	bool m_Local; /**< Whether the item is local. */
	bool m_Abstract; /**< Whether the item is abstract. */
	vector<String> m_Parents; /**< The names of parent configuration
				       items. */
	ExpressionList::Ptr m_ExpressionList; /**< Expressions for this item. */
	DebugInfo m_DebugInfo; /**< Debug information. */
};

}

#endif /* CONFIGITEMBUILDER */
