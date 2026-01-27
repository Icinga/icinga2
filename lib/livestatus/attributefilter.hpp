// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ATTRIBUTEFILTER_H
#define ATTRIBUTEFILTER_H

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

#endif /* FILTER_H */
