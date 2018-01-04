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
class Value
{
public:
	Value() = default;
	Value(std::nullptr_t);
	Value(int value);
	Value(unsigned int value);
	Value(long value);
	Value(unsigned long value);
	Value(long long value);
	Value(unsigned long long value);
	Value(double value);
	Value(bool value);
	Value(const String& value);
	Value(String&& value);
	Value(const char *value);
	Value(const Value& other);
	Value(Value&& other);
	Value(Object *value);
	Value(const intrusive_ptr<Object>& value);

	template<typename T>
	Value(const intrusive_ptr<T>& value)
		: Value(static_pointer_cast<Object>(value))
	{
		static_assert(!std::is_same<T, Object>::value, "T must not be Object");
	}

	bool ToBool() const;

	operator double() const;
	operator String() const;

	Value& operator=(const Value& other);
	Value& operator=(Value&& other);

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
	operator intrusive_ptr<T>() const
	{
		if (IsEmpty() && !IsString())
			return intrusive_ptr<T>();

		if (!IsObject())
			BOOST_THROW_EXCEPTION(std::runtime_error("Cannot convert value of type '" + GetTypeName() + "' to an object."));

		const auto& object = Get<Object::Ptr>();

		ASSERT(object);

		intrusive_ptr<T> tobject = dynamic_pointer_cast<T>(object);

		if (!tobject)
			BOOST_THROW_EXCEPTION(std::bad_cast());

		return tobject;
	}

	bool IsEmpty() const;
	bool IsScalar() const;
	bool IsNumber() const;
	bool IsBoolean() const;
	bool IsString() const;
	bool IsObject() const;

	template<typename T>
	bool IsObjectType() const
	{
		if (!IsObject())
			return false;

		return dynamic_cast<T *>(Get<Object::Ptr>().get());
	}

	ValueType GetType() const;

	void Swap(Value& other);

	String GetTypeName() const;

	Type::Ptr GetReflectionType() const;

	Value Clone() const;

	template<typename T>
	const T& Get() const
	{
		return boost::get<T>(m_Value);
	}

private:
	boost::variant<boost::blank, double, bool, String, Object::Ptr> m_Value;
};

extern template const double& Value::Get<double>() const;
extern template const bool& Value::Get<bool>() const;
extern template const String& Value::Get<String>() const;
extern template const Object::Ptr& Value::Get<Object::Ptr>() const;

extern Value Empty;

Value operator+(const Value& lhs, const char *rhs);
Value operator+(const char *lhs, const Value& rhs);

Value operator+(const Value& lhs, const String& rhs);
Value operator+(const String& lhs, const Value& rhs);

Value operator+(const Value& lhs, const Value& rhs);
Value operator+(const Value& lhs, double rhs);
Value operator+(double lhs, const Value& rhs);
Value operator+(const Value& lhs, int rhs);
Value operator+(int lhs, const Value& rhs);

Value operator-(const Value& lhs, const Value& rhs);
Value operator-(const Value& lhs, double rhs);
Value operator-(double lhs, const Value& rhs);
Value operator-(const Value& lhs, int rhs);
Value operator-(int lhs, const Value& rhs);

Value operator*(const Value& lhs, const Value& rhs);
Value operator*(const Value& lhs, double rhs);
Value operator*(double lhs, const Value& rhs);
Value operator*(const Value& lhs, int rhs);
Value operator*(int lhs, const Value& rhs);

Value operator/(const Value& lhs, const Value& rhs);
Value operator/(const Value& lhs, double rhs);
Value operator/(double lhs, const Value& rhs);
Value operator/(const Value& lhs, int rhs);
Value operator/(int lhs, const Value& rhs);

Value operator%(const Value& lhs, const Value& rhs);
Value operator%(const Value& lhs, double rhs);
Value operator%(double lhs, const Value& rhs);
Value operator%(const Value& lhs, int rhs);
Value operator%(int lhs, const Value& rhs);

Value operator^(const Value& lhs, const Value& rhs);
Value operator^(const Value& lhs, double rhs);
Value operator^(double lhs, const Value& rhs);
Value operator^(const Value& lhs, int rhs);
Value operator^(int lhs, const Value& rhs);

Value operator&(const Value& lhs, const Value& rhs);
Value operator&(const Value& lhs, double rhs);
Value operator&(double lhs, const Value& rhs);
Value operator&(const Value& lhs, int rhs);
Value operator&(int lhs, const Value& rhs);

Value operator|(const Value& lhs, const Value& rhs);
Value operator|(const Value& lhs, double rhs);
Value operator|(double lhs, const Value& rhs);
Value operator|(const Value& lhs, int rhs);
Value operator|(int lhs, const Value& rhs);

Value operator<<(const Value& lhs, const Value& rhs);
Value operator<<(const Value& lhs, double rhs);
Value operator<<(double lhs, const Value& rhs);
Value operator<<(const Value& lhs, int rhs);
Value operator<<(int lhs, const Value& rhs);

Value operator>>(const Value& lhs, const Value& rhs);
Value operator>>(const Value& lhs, double rhs);
Value operator>>(double lhs, const Value& rhs);
Value operator>>(const Value& lhs, int rhs);
Value operator>>(int lhs, const Value& rhs);

bool operator<(const Value& lhs, const Value& rhs);
bool operator<(const Value& lhs, double rhs);
bool operator<(double lhs, const Value& rhs);
bool operator<(const Value& lhs, int rhs);
bool operator<(int lhs, const Value& rhs);

bool operator>(const Value& lhs, const Value& rhs);
bool operator>(const Value& lhs, double rhs);
bool operator>(double lhs, const Value& rhs);
bool operator>(const Value& lhs, int rhs);
bool operator>(int lhs, const Value& rhs);

bool operator<=(const Value& lhs, const Value& rhs);
bool operator<=(const Value& lhs, double rhs);
bool operator<=(double lhs, const Value& rhs);
bool operator<=(const Value& lhs, int rhs);
bool operator<=(int lhs, const Value& rhs);

bool operator>=(const Value& lhs, const Value& rhs);
bool operator>=(const Value& lhs, double rhs);
bool operator>=(double lhs, const Value& rhs);
bool operator>=(const Value& lhs, int rhs);
bool operator>=(int lhs, const Value& rhs);

std::ostream& operator<<(std::ostream& stream, const Value& value);
std::istream& operator>>(std::istream& stream, Value& value);

}

extern template class boost::variant<boost::blank, double, bool, icinga::String, icinga::Object::Ptr>;

#endif /* VALUE_H */
