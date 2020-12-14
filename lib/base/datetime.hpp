/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "base/datetime-ti.hpp"
#include "base/value.hpp"
#include <vector>

namespace icinga
{

/**
 * A date/time value.
 *
 * @ingroup base
 */
class DateTime final : public ObjectImpl<DateTime>
{
public:
	DECLARE_OBJECT(DateTime);

	DateTime(double value);
	DateTime(const std::vector<Value>& args);

	String Format(const String& format) const;

	double GetValue() const override;
	String ToString() const override;

	static Object::Ptr GetPrototype();

private:
	double m_Value;
};

}
