/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef DATETIME_H
#define DATETIME_H

#include "base/i2-base.hpp"
#include "base/datetime.thpp"
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
