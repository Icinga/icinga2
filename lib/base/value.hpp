/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

namespace icinga
{

typedef double Timestamp;

/**
 * The type of a Value.
 *
 * @ingroup base
 */
enum ValueType
{
	ValueEmpty = 0,
	ValueNumber = 1,
	ValueBoolean = 2,
	ValueString = 3,
	ValueObject = 4
};

/**
 * A type that can hold an arbitrary value.
 *
 * @ingroup base
 */
class I2_BASE_API Value
{
public:
	inline Value(void)
	{ }

	inline Value(int value)
		: m_Value(double(value))
	{ }

	inline Value(unsigned int value)
		: m_Value(double(value))
	{ }

	inline Value(long value)
		: m_Value(double(value))
	{ }

	inline Value(unsigned long value)
		: m_Value(double(value))
	{ }

	inline Value(long long value)
		: m_Value(double(value))
	{ }

	inline Value(unsigned long long value)
		: m_Value(double(value))
	{ }

	inline Value(double value)
		: m_Value(value)
	{ }

	inline Value(bool value)
		: m_Value(value)
	{ }

	inline Value(const String& value)
		: m_Value(value)
	{ }

	inline Value(String&& value)
		: m_Value(value)
	{ }

	inline Value(const char *value)
		: m_Value(String(value))
	{ }

	Value(const Value& other)
		: m_Value(other.m_Value)
	{ }

	Value(Value&& other)
	{
#if BOOST_VERSION >= 105400
		m_Value = std::move(other.m_Value);
#else /* BOOST_VERSION */
		m_Value.swap(other.m_Value);
#endif /* BOOST_VERSION */
	}

	inline Value(Object *value)
	{
		if (!value)
			return;

		m_Value = Object::Ptr(value);
	}

	template<typename T>
	inline Value(const intrusive_ptr<T>& value)
	{
		if (!value)
			return;

		m_Value = static_pointer_cast<Object>(value);
	}

	bool ToBool(void) const;

	operator double(void) const;
	operator String(void) const;

	Value& operator=(const Value& other)
	{
		m_Value = other.m_Value;
		return *this;
	}

	Value& operator=(Value&& other)
	{
#if BOOST_VERSION >= 105400
		m_Value = std::move(other.m_Value);
#else /* BOOST_VERSION */
		m_Value.swap(other.m_Value);
#endif /* BOOST_VERSION */

		return *this;
	}

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
	operator intrusive_ptr<T>(void) const
	{
		if (IsEmpty() && !IsString())
			return intrusive_ptr<T>();

		if (!IsObject())
			BOOST_THROW_EXCEPTION(std::runtime_error("Cannot convert value of type '" + GetTypeName() + "' to an object."));

		const Object::Ptr& object = boost::get<Object::Ptr>(m_Value);

		ASSERT(object);

		intrusive_ptr<T> tobject = dynamic_pointer_cast<T>(object);

		if (!tobject)
			BOOST_THROW_EXCEPTION(std::bad_cast());

		return tobject;
	}

	/**
	* Checks whether the variant is empty.
	*
	* @returns true if the variant is empty, false otherwise.
	*/
	inline bool IsEmpty(void) const
	{
		return (GetType() == ValueEmpty || (IsString() && boost::get<String>(m_Value).IsEmpty()));
	}

	/**
	* Checks whether the variant is scalar (i.e. not an object and not empty).
	*
	* @returns true if the variant is scalar, false otherwise.
	*/
	inline bool IsScalar(void) const
	{
		return !IsEmpty() && !IsObject();
	}

	/**
	* Checks whether the variant is a number.
	*
	* @returns true if the variant is a number.
	*/
	inline bool IsNumber(void) const
	{
		return (GetType() == ValueNumber);
	}

	/**
	 * Checks whether the variant is a boolean.
	 *
	 * @returns true if the variant is a boolean.
	 */
	inline bool IsBoolean(void) const
	{
		return (GetType() == ValueBoolean);
	}

	/**
	* Checks whether the variant is a string.
	*
	* @returns true if the variant is a string.
	*/
	inline bool IsString(void) const
	{
		return (GetType() == ValueString);
	}

	/**
	* Checks whether the variant is a non-null object.
	*
	* @returns true if the variant is a non-null object, false otherwise.
	*/
	inline bool IsObject(void) const
	{
		return  (GetType() == ValueObject);
	}

	template<typename T>
	bool IsObjectType(void) const
	{
		if (!IsObject())
			return false;

		return (dynamic_cast<T *>(boost::get<Object::Ptr>(m_Value).get()) != NULL);
	}

	/**
	* Returns the type of the value.
	*
	* @returns The type.
	*/
	inline ValueType GetType(void) const
	{
		return static_cast<ValueType>(m_Value.which());
	}

	inline void Swap(Value& other)
	{
		m_Value.swap(other.m_Value);
	}

	String GetTypeName(void) const;

	Type::Ptr GetReflectionType(void) const;
	
	Value Clone(void) const;

	template<typename T>
	const T& Get(void) const
	{
		return boost::get<T>(m_Value);
	}

private:
	boost::variant<boost::blank, double, bool, String, Object::Ptr> m_Value;
};

extern I2_BASE_API Value Empty;

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

I2_BASE_API Value operator%(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator%(const Value& lhs, double rhs);
I2_BASE_API Value operator%(double lhs, const Value& rhs);
I2_BASE_API Value operator%(const Value& lhs, int rhs);
I2_BASE_API Value operator%(int lhs, const Value& rhs);

I2_BASE_API Value operator^(const Value& lhs, const Value& rhs);
I2_BASE_API Value operator^(const Value& lhs, double rhs);
I2_BASE_API Value operator^(double lhs, const Value& rhs);
I2_BASE_API Value operator^(const Value& lhs, int rhs);
I2_BASE_API Value operator^(int lhs, const Value& rhs);

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
