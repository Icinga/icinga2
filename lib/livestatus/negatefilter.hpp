// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef NEGATEFILTER_H
#define NEGATEFILTER_H

#include "livestatus/filter.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class NegateFilter final : public Filter
{
public:
	DECLARE_PTR_TYPEDEFS(NegateFilter);

	NegateFilter(Filter::Ptr inner);

	bool Apply(const Table::Ptr& table, const Value& row) override;

private:
	Filter::Ptr m_Inner;
};

}

#endif /* NEGATEFILTER_H */
