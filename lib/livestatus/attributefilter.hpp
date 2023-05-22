/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/filter.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class AttributeFilter final : public Filter
{
public:
	DECLARE_PTR_TYPEDEFS(AttributeFilter);

	AttributeFilter(String column, String op, String operand);

	bool Apply(const Table::Ptr& table, const Value& row) override;

protected:
	String m_Column;
	String m_Operator;
	String m_Operand;
};

}
