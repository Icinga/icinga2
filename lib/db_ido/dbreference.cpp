/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "dbreference.hpp"

using namespace icinga;

DbReference::DbReference(long id)
	: m_Id(id)
{ }

bool DbReference::IsValid() const
{
	return (m_Id != -1);
}

DbReference::operator long() const
{
	return m_Id;
}
