#ifndef PERFDATAVALUE_H
#define PERFDATAVALUE_H

#include "icinga/i2-icinga.h"
#include "icinga/perfdatavalue.th"

namespace icinga
{

class I2_ICINGA_API PerfdataValue : public ObjectImpl<PerfdataValue>
{
public:
	DECLARE_PTR_TYPEDEFS(PerfdataValue);

	PerfdataValue(double value, bool counter, const String& unit,
	    const Value& warn = Empty, const Value& crit = Empty,
	    const Value& min = Empty, const Value& max = Empty);

	static Value Parse(const String& perfdata);
	static String Format(const Value& perfdata);
};

}

#endif /* PERFDATA_VALUE */
