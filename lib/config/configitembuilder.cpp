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

#include "config/configitembuilder.hpp"
#include "base/dynamictype.hpp"
#include <sstream>
#include <boost/foreach.hpp>

using namespace icinga;

ConfigItemBuilder::ConfigItemBuilder(void)
	: m_Abstract(false), m_Expressions(new Array())
{
	m_DebugInfo.FirstLine = 0;
	m_DebugInfo.FirstColumn = 0;
	m_DebugInfo.LastLine = 0;
	m_DebugInfo.LastColumn = 0;
}

ConfigItemBuilder::ConfigItemBuilder(const DebugInfo& debugInfo)
	: m_Abstract(false), m_Expressions(new Array())
{
	m_DebugInfo = debugInfo;
}

void ConfigItemBuilder::SetType(const String& type)
{
	m_Type = type;
}

void ConfigItemBuilder::SetName(const String& name)
{
	m_Name = name;
}

void ConfigItemBuilder::SetAbstract(bool abstract)
{
	m_Abstract = abstract;
}

void ConfigItemBuilder::SetScope(const Object::Ptr& scope)
{
	m_Scope = scope;
}

void ConfigItemBuilder::SetZone(const String& zone)
{
	m_Zone = zone;
}

void ConfigItemBuilder::AddExpression(const Expression::Ptr& expr)
{
	m_Expressions->Add(expr);
}

ConfigItem::Ptr ConfigItemBuilder::Compile(void)
{
	if (m_Type.IsEmpty()) {
		std::ostringstream msgbuf;
		msgbuf << "The type name of an object may not be empty: " << m_DebugInfo;
		BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
	}

	if (!DynamicType::GetByName(m_Type)) {
		std::ostringstream msgbuf;
		msgbuf << "The type '" + m_Type + "' is unknown: " << m_DebugInfo;
		BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
	}

	if (m_Name.IsEmpty()) {
		std::ostringstream msgbuf;
		msgbuf << "The name of an object may not be empty: " << m_DebugInfo;
		BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
	}

	Array::Ptr exprs = new Array();
	Array::Ptr templateArray = new Array();
	templateArray->Add(m_Name);

	exprs->Add(new Expression(&Expression::OpSet,
	    MakeArray(MakeArray(MakeLiteral("templates")), OpSetAdd),
	    new Expression(&Expression::OpLiteral, templateArray, m_DebugInfo),
	    m_DebugInfo));

	exprs->Add(new Expression(&Expression::OpDict, m_Expressions, true, m_DebugInfo));
	
	Expression::Ptr exprl = new Expression(&Expression::OpDict, exprs, true, m_DebugInfo);

	return new ConfigItem(m_Type, m_Name, m_Abstract, exprl,
	    m_DebugInfo, m_Scope, m_Zone);
}
