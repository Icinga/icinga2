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

class I2_DYN_API ConfigItemBuilder : public Object
{
public:
	typedef shared_ptr<ConfigItemBuilder> Ptr;
	typedef weak_ptr<ConfigItemBuilder> WeakPtr;

	ConfigItemBuilder(void);
	ConfigItemBuilder(const DebugInfo& debugInfo);

	void SetType(const String& type);
	void SetName(const String& name);
	void SetLocal(bool local);
	void SetAbstract(bool abstract);

	void AddParent(const String& parent);

	void AddExpression(const Expression& expr);
	void AddExpression(const String& key, ExpressionOperator op, const Value& value);
	void AddExpressionList(const ExpressionList::Ptr& exprl);

	ConfigItem::Ptr Compile(void);

private:
	String m_Type;
	String m_Name;
	bool m_Local;
	bool m_Abstract;
	vector<String> m_Parents;
	ExpressionList::Ptr m_ExpressionList;
	DebugInfo m_DebugInfo;
};

}

#endif /* CONFIGITEMBUILDER */
