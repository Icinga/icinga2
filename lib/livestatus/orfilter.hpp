// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ORFILTER_H
#define ORFILTER_H

#include "livestatus/combinerfilter.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class OrFilter final : public CombinerFilter
{
public:
	DECLARE_PTR_TYPEDEFS(OrFilter);

	bool Apply(const Table::Ptr& table, const Value& row) override;
};

}

#endif /* ORFILTER_H */
