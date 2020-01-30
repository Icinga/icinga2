/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
