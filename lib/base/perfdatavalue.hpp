// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PERFDATAVALUE_H
#define PERFDATAVALUE_H

#include "base/i2-base.hpp"
#include "base/perfdatavalue-ti.hpp"

namespace icinga
{

/**
 * A performance data value.
 *
 * @ingroup base
 */
class PerfdataValue final : public ObjectImpl<PerfdataValue>
{
public:
	DECLARE_OBJECT(PerfdataValue);

	PerfdataValue() = default;

	PerfdataValue(const String& label, double value, bool counter = false, const String& unit = "",
		const Value& warn = Empty, const Value& crit = Empty,
		const Value& min = Empty, const Value& max = Empty);

	static PerfdataValue::Ptr Parse(const String& perfdata);
	String Format() const;

private:
	static Value ParseWarnCritMinMaxToken(const std::vector<String>& tokens,
		std::vector<String>::size_type index, const String& description);
};

}

#endif /* PERFDATA_VALUE */
