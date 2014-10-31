/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef VALUE_H
#define VALUE_H

#include "base/object.hpp"
#include "base/string.hpp"
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>

struct cJSON;

namespace icinga
{

/**
 * The type of a Value.
 *
 * @ingroup base
 */
enum ValueType
{
	ValueEmpty = 0,
	ValueNumber = 1,
	ValueString = 2,
	ValueObject = 3
};

/**
 * A type that can hold an arbitrary value.
 *
 * @ingroup base
 */
class I2_BASE_API Value
{
public:
	Value(void);
	Value(int value);
	Value(unsigned int value);
	Value(long value);
	Value(unsigned long value);
	Value(double value);
	Value(const String& value);
	Value(const char *value);

	template<typename T>
	inline Value(const shared_ptr<T>& value)
		: m_Value()
	{
		if (!value)
			return;

		m_Value = static_pointer_cast<Object>(value);
	}

	bool ToBool(void) const;

	operator double(void) const;
	operator String(void) const;

	bool operator==(bool rhs) const;
	bool operator!=(bool rhs) const;

	bool operator==(int rhs) const;
	bool operator!=(int rhs) const;

	bool operator==(double rhs) const;
	bool operator!=(double rhs) const;

	bool operator==(const char *rhs) const;
	bool operator!=(const char *rhs) const;

	bool operator==(const String& rhs) const;
	bool operator!=(const String& rhs) const;

	bool operator==(const Value& rhs) const;
	bool operator!=(const Value& rhs) const;

	template<typename T>
	operator shared_ptr<T>(void) const
	{
		if (IsEmpty())
			return shared_ptr<T>();

		shared_ptr<T> object = dynamic_pointer_cast<T>(boost::get<Object::Ptr>(m_Value));

		if (!object)
			BOOST_THROW_EXCEPTION(std::bad_cast());

		return object;
	}

	bool IsEmpty(void) const;
	bool IsScalar(void) const;
	bool IsNumber(void) const;
	bool IsString(void) const;
	bool IsObject(void) const;

	template<typename T>
	bool IsObjectType(void) const
	{
		if (!IsObject())
			return false;

		return (dynamic_pointer_cast<T>(boost::get<Object::Ptr>(m_Value)) != NULL);
	}

	ValueType GetType(void) const;
	String GetTypeName(void) const;

private:
	boost::variant<boost::blank, double, String, Object::Ptr> m_Value;
};

static Value Empty;

I2_BASE_API Value operator+(const Value& lhs, const char *rhs);
I2_BASE_API Value operator+(const char *lhs, const Value& rhs);

I2_BASE_API Value operator+(const Value& lhs, const String& rhs);
I2_BASE_API Value operator+(const String& lhs, const Value& rhs);

I2_BASE_API Value operator+(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator+(const Value& lhs, double rhs);
I2_BASE_API Value operator+(double lhs, const Value& rhs);
I2_BASE_API Value operator+(const Value& lhs, int rhs);
I2_BASE_API Value operator+(int lhs, const Value& rhs);

I2_BASE_API Value operator-(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator-(const Value& lhs, double rhs);
I2_BASE_API Value operator-(double lhs, const Value& rhs);
I2_BASE_API Value operator-(const Value& lhs, int rhs);
I2_BASE_API Value operator-(int lhs, const Value& rhs);

I2_BASE_API Value operator*(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator*(const Value& lhs, double rhs);
I2_BASE_API Value operator*(double lhs, const Value& rhs);
I2_BASE_API Value operator*(const Value& lhs, int rhs);
I2_BASE_API Value operator*(int lhs, const Value& rhs);

I2_BASE_API Value operator/(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator/(const Value& lhs, double rhs);
I2_BASE_API Value operator/(double lhs, const Value& rhs);
I2_BASE_API Value operator/(const Value& lhs, int rhs);
I2_BASE_API Value operator/(int lhs, const Value& rhs);

I2_BASE_API Value operator&(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator&(const Value& lhs, double rhs);
I2_BASE_API Value operator&(double lhs, const Value& rhs);
I2_BASE_API Value operator&(const Value& lhs, int rhs);
I2_BASE_API Value operator&(int lhs, const Value& rhs);

I2_BASE_API Value operator|(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator|(const Value& lhs, double rhs);
I2_BASE_API Value operator|(double lhs, const Value& rhs);
I2_BASE_API Value operator|(const Value& lhs, int rhs);
I2_BASE_API Value operator|(int lhs, const Value& rhs);

I2_BASE_API Value operator<<(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator<<(const Value& lhs, double rhs);
I2_BASE_API Value operator<<(double lhs, const Value& rhs);
I2_BASE_API Value operator<<(const Value& lhs, int rhs);
I2_BASE_API Value operator<<(int lhs, const Value& rhs);

I2_BASE_API Value operator>>(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator>>(const Value& lhs, double rhs);
I2_BASE_API Value operator>>(double lhs, const Value& rhs);
I2_BASE_API Value operator>>(const Value& lhs, int rhs);
I2_BASE_API Value operator>>(int lhs, const Value& rhs);

I2_BASE_API bool operator<(const Value& lhs, const Value& rhs);
I2_BASE_API bool operator<(const Value& lhs, double rhs);
I2_BASE_API bool operator<(double lhs, const Value& rhs);
I2_BASE_API bool operator<(const Value& lhs, int rhs);
I2_BASE_API bool operator<(int lhs, const Value& rhs);

I2_BASE_API bool operator>(const Value& lhs, const Value& rhs);
I2_BASE_API bool operator>(const Value& lhs, double rhs);
I2_BASE_API bool operator>(double lhs, const Value& rhs);
I2_BASE_API bool operator>(const Value& lhs, int rhs);
I2_BASE_API bool operator>(int lhs, const Value& rhs);

I2_BASE_API bool operator<=(const Value& lhs, const Value& rhs);
I2_BASE_API bool operator<=(const Value& lhs, double rhs);
I2_BASE_API bool operator<=(double lhs, const Value& rhs);
I2_BASE_API bool operator<=(const Value& lhs, int rhs);
I2_BASE_API bool operator<=(int lhs, const Value& rhs);

I2_BASE_API bool operator>=(const Value& lhs, const Value& rhs);
I2_BASE_API bool operator>=(const Value& lhs, double rhs);
I2_BASE_API bool operator>=(double lhs, const Value& rhs);
I2_BASE_API bool operator>=(const Value& lhs, int rhs);
I2_BASE_API bool operator>=(int lhs, const Value& rhs);

I2_BASE_API std::ostream& operator<<(std::ostream& stream, const Value& value);
I2_BASE_API std::istream& operator>>(std::istream& stream, Value& value);

}

#endif /* VALUE_H */
