#include "i2-dyn.h"

using namespace icinga;

ExpressionList::Ptr DConfigObject::GetExpressionList(void) const
{
	return m_ExpressionList;
}

void DConfigObject::SetExpressionList(const ExpressionList::Ptr& exprl)
{
	m_ExpressionList = exprl;
}

vector<string> DConfigObject::GetParents(void) const
{
	return m_Parents;
}

void DConfigObject::AddParent(string parent)
{
	m_Parents.push_back(parent);
}
