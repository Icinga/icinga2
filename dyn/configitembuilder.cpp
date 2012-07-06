#include "i2-dyn.h"

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

void ConfigItemBuilder::SetType(const string& type)
{
	m_Type = type;
}

void ConfigItemBuilder::SetName(const string& name)
{
	m_Name = name;
}

void ConfigItemBuilder::SetLocal(bool local)
{
	m_Local = local;
}

void ConfigItemBuilder::SetAbstract(bool abstract)
{
	m_Abstract = abstract;
}

void ConfigItemBuilder::AddParent(const string& parent)
{
	m_Parents.push_back(parent);
}

void ConfigItemBuilder::AddExpression(const Expression& expr)
{
	m_ExpressionList->AddExpression(expr);
}

void ConfigItemBuilder::AddExpression(const string& key, ExpressionOperator op, const Variant& value)
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
	assert(!m_Type.empty());
	assert(!m_Name.empty());

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

	return boost::make_shared<ConfigItem>(m_Type, m_Name, exprl, m_Parents, m_DebugInfo);
}
