// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DATETIME_H
#define DATETIME_H

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

#endif /* DATETIME_H */
