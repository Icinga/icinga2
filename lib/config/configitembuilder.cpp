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

#include "i2-config.h"

using namespace icinga;

ConfigItemBuilder::ConfigItemBuilder(void)
	: m_Local(false), m_Abstract(false),
	  m_ExpressionList(boost::make_shared<ExpressionList>())
{
	m_DebugInfo.FirstLine = 0;
	m_DebugInfo.FirstColumn = 0;
	m_DebugInfo.LastLine = 0;
	m_DebugInfo.LastColumn = 0;
}

ConfigItemBuilder::ConfigItemBuilder(const DebugInfo& debugInfo)
	: m_Local(false), m_Abstract(false),
	  m_ExpressionList(boost::make_shared<ExpressionList>())
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

void ConfigItemBuilder::SetUnit(const String& unit)
{
	m_Unit = unit;
}

void ConfigItemBuilder::SetLocal(bool local)
{
	m_Local = local;
}

void ConfigItemBuilder::SetAbstract(bool abstract)
{
	m_Abstract = abstract;
}

void ConfigItemBuilder::AddParent(const String& parent)
{
	m_Parents.push_back(parent);
}

void ConfigItemBuilder::AddExpression(const Expression& expr)
{
	m_ExpressionList->AddExpression(expr);
}

void ConfigItemBuilder::AddExpression(const String& key, ExpressionOperator op,
    const Value& value)
{
	Expression expr(key, op, value, m_DebugInfo);
	AddExpression(expr);
}

void ConfigItemBuilder::AddExpressionList(const ExpressionList::Ptr& exprl)
{
	AddExpression("", OperatorExecute, exprl);
}

ConfigItem::Ptr ConfigItemBuilder::Compile(void)
{
	if (m_Type.IsEmpty()) {
		stringstream msgbuf;
		msgbuf << "The type name of an object may not be empty: " << m_DebugInfo;
		BOOST_THROW_EXCEPTION(invalid_argument(msgbuf.str()));
	}

	if (!DynamicType::GetByName(m_Type)) {
		stringstream msgbuf;
		msgbuf << "The type '" + m_Type + "' is unknown: " << m_DebugInfo;
		BOOST_THROW_EXCEPTION(invalid_argument(msgbuf.str()));
	}

	if (m_Name.IsEmpty()) {
		stringstream msgbuf;
		msgbuf << "The name of an object may not be empty: " << m_DebugInfo;
		BOOST_THROW_EXCEPTION(invalid_argument(msgbuf.str()));
	}

	BOOST_FOREACH(const String& parent, m_Parents) {
		ConfigItem::Ptr item;

		ConfigCompilerContext *context = ConfigCompilerContext::GetContext();

		if (context)
			item = context->GetItem(m_Type, parent);

		/* ignore already active objects while we're in the compiler
		 * context and linking to existing items is disabled. */
		if (!item && (!context || (context->GetFlags() & CompilerLinkExisting)))
			item = ConfigItem::GetObject(m_Type, parent);

		if (!item) {
			stringstream msgbuf;
			msgbuf << "The parent config item '" + parent + "' does not exist: " << m_DebugInfo;
			BOOST_THROW_EXCEPTION(invalid_argument(msgbuf.str()));
		}
	}

	ExpressionList::Ptr exprl = boost::make_shared<ExpressionList>();

	Expression execExpr("", OperatorExecute, m_ExpressionList, m_DebugInfo);
	exprl->AddExpression(execExpr);

	Expression typeExpr("__type", OperatorSet, m_Type, m_DebugInfo);
	exprl->AddExpression(typeExpr);

	Expression nameExpr("__name", OperatorSet, m_Name, m_DebugInfo);
	exprl->AddExpression(nameExpr);

	Expression localExpr("__local", OperatorSet, m_Local, m_DebugInfo);
	exprl->AddExpression(localExpr);

	Expression abstractExpr("__abstract", OperatorSet, m_Abstract, m_DebugInfo);
	exprl->AddExpression(abstractExpr);

	return boost::make_shared<ConfigItem>(m_Type, m_Name, m_Unit, exprl, m_Parents,
	    m_DebugInfo);
}
