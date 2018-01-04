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

#include "base/value.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/datetime.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include <boost/lexical_cast.hpp>

using namespace icinga;

Value::operator double() const
{
	const double *value = boost::get<double>(&m_Value);

	if (value)
		return *value;

	const bool *fvalue = boost::get<bool>(&m_Value);

	if (fvalue)
		return *fvalue;

	if (IsEmpty())
		return 0;

	try {
		return boost::lexical_cast<double>(m_Value);
	} catch (const std::exception&) {
		std::ostringstream msgbuf;
		msgbuf << "Can't convert '" << *this << "' to a floating point number.";
		BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
	}
}

Value::operator String() const
{
	Object *object;

	switch (GetType()) {
		case ValueEmpty:
			return String();
		case ValueNumber:
			return Convert::ToString(boost::get<double>(m_Value));
		case ValueBoolean:
			if (boost::get<bool>(m_Value))
				return "true";
			else
				return "false";
		case ValueString:
			return boost::get<String>(m_Value);
		case ValueObject:
			object = boost::get<Object::Ptr>(m_Value).get();
			return object->ToString();
		default:
			BOOST_THROW_EXCEPTION(std::runtime_error("Unknown value type."));
	}
}

std::ostream& icinga::operator<<(std::ostream& stream, const Value& value)
{
	if (value.IsBoolean())
		stream << static_cast<int>(value);
	else
		stream << static_cast<String>(value);

	return stream;
}

std::istream& icinga::operator>>(std::istream& stream, Value& value)
{
	String tstr;
	stream >> tstr;
	value = tstr;
	return stream;
}

bool Value::operator==(bool rhs) const
{
	return *this == Value(rhs);
}

bool Value::operator!=(bool rhs) const
{
	return !(*this == rhs);
}

bool Value::operator==(int rhs) const
{
	return *this == Value(rhs);
}

bool Value::operator!=(int rhs) const
{
	return !(*this == rhs);
}

bool Value::operator==(double rhs) const
{
	return *this == Value(rhs);
}

bool Value::operator!=(double rhs) const
{
	return !(*this == rhs);
}

bool Value::operator==(const char *rhs) const
{
	return static_cast<String>(*this) == rhs;
}

bool Value::operator!=(const char *rhs) const
{
	return !(*this == rhs);
}

bool Value::operator==(const String& rhs) const
{
	return static_cast<String>(*this) == rhs;
}

bool Value::operator!=(const String& rhs) const
{
	return !(*this == rhs);
}

bool Value::operator==(const Value& rhs) const
{
	if (IsNumber() && rhs.IsNumber())
		return Get<double>() == rhs.Get<double>();
	else if ((IsBoolean() || IsNumber()) && (rhs.IsBoolean() || rhs.IsNumber()) && !(IsEmpty() && rhs.IsEmpty()))
		return static_cast<double>(*this) == static_cast<double>(rhs);

	if (IsString() && rhs.IsString())
		return Get<String>() == rhs.Get<String>();
	else if ((IsString() || IsEmpty()) && (rhs.IsString() || rhs.IsEmpty()) && !(IsEmpty() && rhs.IsEmpty()))
		return static_cast<String>(*this) == static_cast<String>(rhs);

	if (IsEmpty() != rhs.IsEmpty())
		return false;

	if (IsEmpty())
		return true;

	if (IsObject() != rhs.IsObject())
		return false;

	if (IsObject()) {
		if (IsObjectType<DateTime>() && rhs.IsObjectType<DateTime>()) {
			DateTime::Ptr dt1 = *this;
			DateTime::Ptr dt2 = rhs;

			return dt1->GetValue() == dt2->GetValue();
		}

		if (IsObjectType<Array>() && rhs.IsObjectType<Array>()) {
			Array::Ptr arr1 = *this;
			Array::Ptr arr2 = rhs;

			if (arr1 == arr2)
				return true;

			if (arr1->GetLength() != arr2->GetLength())
				return false;

			for (Array::SizeType i = 0; i < arr1->GetLength(); i++) {
				if (arr1->Get(i) != arr2->Get(i))
					return false;
			}

			return true;
		}

		return Get<Object::Ptr>() == rhs.Get<Object::Ptr>();
	}

	return false;
}

bool Value::operator!=(const Value& rhs) const
{
	return !(*this == rhs);
}

Value icinga::operator+(const Value& lhs, const char *rhs)
{
	return lhs + Value(rhs);
}

Value icinga::operator+(const char *lhs, const Value& rhs)
{
	return Value(lhs) + rhs;
}

Value icinga::operator+(const Value& lhs, const String& rhs)
{
	return lhs + Value(rhs);
}

Value icinga::operator+(const String& lhs, const Value& rhs)
{
	return Value(lhs) + rhs;
}

Value icinga::operator+(const Value& lhs, const Value& rhs)
{
	if ((lhs.IsEmpty() || lhs.IsNumber()) && !lhs.IsString() && (rhs.IsEmpty() || rhs.IsNumber()) && !rhs.IsString() && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<double>(lhs) + static_cast<double>(rhs);
	if ((lhs.IsString() || lhs.IsEmpty() || lhs.IsNumber()) && (rhs.IsString() || rhs.IsEmpty() || rhs.IsNumber()) && (!(lhs.IsEmpty() && rhs.IsEmpty()) || lhs.IsString() || rhs.IsString()))
		return static_cast<String>(lhs) + static_cast<String>(rhs);
	else if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<double>(lhs) + static_cast<double>(rhs);
	else if (lhs.IsObjectType<DateTime>() && rhs.IsNumber())
		return new DateTime(Convert::ToDateTimeValue(lhs) + rhs);
	else if ((lhs.IsObjectType<Array>() || lhs.IsEmpty()) && (rhs.IsObjectType<Array>() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty())) {
		Array::Ptr result = new Array();
		if (!lhs.IsEmpty())
			static_cast<Array::Ptr>(lhs)->CopyTo(result);
		if (!rhs.IsEmpty())
			static_cast<Array::Ptr>(rhs)->CopyTo(result);
		return result;
	} else if ((lhs.IsObjectType<Dictionary>() || lhs.IsEmpty()) && (rhs.IsObjectType<Dictionary>() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty())) {
		Dictionary::Ptr result = new Dictionary();
		if (!lhs.IsEmpty())
			static_cast<Dictionary::Ptr>(lhs)->CopyTo(result);
		if (!rhs.IsEmpty())
			static_cast<Dictionary::Ptr>(rhs)->CopyTo(result);
		return result;
	} else {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator + cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
	}
}

Value icinga::operator+(const Value& lhs, double rhs)
{
	return lhs + Value(rhs);
}

Value icinga::operator+(double lhs, const Value& rhs)
{
	return Value(lhs) + rhs;
}

Value icinga::operator+(const Value& lhs, int rhs)
{
	return lhs + Value(rhs);
}

Value icinga::operator+(int lhs, const Value& rhs)
{
	return Value(lhs) + rhs;
}

Value icinga::operator-(const Value& lhs, const Value& rhs)
{
	if ((lhs.IsNumber() || lhs.IsEmpty()) && !lhs.IsString() && (rhs.IsNumber() || rhs.IsEmpty()) && !rhs.IsString() && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<double>(lhs) - static_cast<double>(rhs);
	else if (lhs.IsObjectType<DateTime>() && rhs.IsNumber())
		return new DateTime(Convert::ToDateTimeValue(lhs) - rhs);
	else if (lhs.IsObjectType<DateTime>() && rhs.IsObjectType<DateTime>())
		return Convert::ToDateTimeValue(lhs) - Convert::ToDateTimeValue(rhs);
	else if ((lhs.IsObjectType<DateTime>() || lhs.IsEmpty()) && (rhs.IsObjectType<DateTime>() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return new DateTime(Convert::ToDateTimeValue(lhs) - Convert::ToDateTimeValue(rhs));
	else if ((lhs.IsObjectType<Array>() || lhs.IsEmpty()) && (rhs.IsObjectType<Array>() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty())) {
		if (lhs.IsEmpty())
			return new Array();

		Array::Ptr result = new Array();
		Array::Ptr left = lhs;
		Array::Ptr right = rhs;

		ObjectLock olock(left);
		for (const Value& lv : left) {
			bool found = false;
			ObjectLock xlock(right);
			for (const Value& rv : right) {
				if (lv == rv) {
					found = true;
					break;
				}
			}

			if (found)
				continue;

			result->Add(lv);
		}

		return result;
	} else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator - cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

Value icinga::operator-(const Value& lhs, double rhs)
{
	return lhs - Value(rhs);
}

Value icinga::operator-(double lhs, const Value& rhs)
{
	return Value(lhs) - rhs;
}

Value icinga::operator-(const Value& lhs, int rhs)
{
	return lhs - Value(rhs);
}

Value icinga::operator-(int lhs, const Value& rhs)
{
	return Value(lhs) - rhs;
}

Value icinga::operator*(const Value& lhs, const Value& rhs)
{
	if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<double>(lhs) * static_cast<double>(rhs);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator * cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

Value icinga::operator*(const Value& lhs, double rhs)
{
	return lhs * Value(rhs);
}

Value icinga::operator*(double lhs, const Value& rhs)
{
	return Value(lhs) * rhs;
}

Value icinga::operator*(const Value& lhs, int rhs)
{
	return lhs * Value(rhs);
}

Value icinga::operator*(int lhs, const Value& rhs)
{
	return Value(lhs) * rhs;
}

Value icinga::operator/(const Value& lhs, const Value& rhs)
{
	if (rhs.IsEmpty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Right-hand side argument for operator / is Empty."));
	else if ((lhs.IsEmpty() || lhs.IsNumber()) && rhs.IsNumber()) {
		if (static_cast<double>(rhs) == 0)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Right-hand side argument for operator / is 0."));

		return static_cast<double>(lhs) / static_cast<double>(rhs);
	} else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator / cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

Value icinga::operator/(const Value& lhs, double rhs)
{
	return lhs / Value(rhs);
}

Value icinga::operator/(double lhs, const Value& rhs)
{
	return Value(lhs) / rhs;
}

Value icinga::operator/(const Value& lhs, int rhs)
{
	return lhs / Value(rhs);
}

Value icinga::operator/(int lhs, const Value& rhs)
{
	return Value(lhs) / rhs;
}

Value icinga::operator%(const Value& lhs, const Value& rhs)
{
	if (rhs.IsEmpty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Right-hand side argument for operator % is Empty."));
	else if ((rhs.IsNumber() || lhs.IsNumber()) && rhs.IsNumber()) {
		if (static_cast<double>(rhs) == 0)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Right-hand side argument for operator % is 0."));

		return static_cast<int>(lhs) % static_cast<int>(rhs);
	} else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator % cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

Value icinga::operator%(const Value& lhs, double rhs)
{
	return lhs % Value(rhs);
}

Value icinga::operator%(double lhs, const Value& rhs)
{
	return Value(lhs) % rhs;
}

Value icinga::operator%(const Value& lhs, int rhs)
{
	return lhs % Value(rhs);
}

Value icinga::operator%(int lhs, const Value& rhs)
{
	return Value(lhs) % rhs;
}

Value icinga::operator^(const Value& lhs, const Value& rhs)
{
	if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<int>(lhs) ^ static_cast<int>(rhs);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator & cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

Value icinga::operator^(const Value& lhs, double rhs)
{
	return lhs ^ Value(rhs);
}

Value icinga::operator^(double lhs, const Value& rhs)
{
	return Value(lhs) ^ rhs;
}

Value icinga::operator^(const Value& lhs, int rhs)
{
	return lhs ^ Value(rhs);
}

Value icinga::operator^(int lhs, const Value& rhs)
{
	return Value(lhs) ^ rhs;
}

Value icinga::operator&(const Value& lhs, const Value& rhs)
{
	if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<int>(lhs) & static_cast<int>(rhs);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator & cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

Value icinga::operator&(const Value& lhs, double rhs)
{
	return lhs & Value(rhs);
}

Value icinga::operator&(double lhs, const Value& rhs)
{
	return Value(lhs) & rhs;
}

Value icinga::operator&(const Value& lhs, int rhs)
{
	return lhs & Value(rhs);
}

Value icinga::operator&(int lhs, const Value& rhs)
{
	return Value(lhs) & rhs;
}

Value icinga::operator|(const Value& lhs, const Value& rhs)
{
	if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<int>(lhs) | static_cast<int>(rhs);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator | cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

Value icinga::operator|(const Value& lhs, double rhs)
{
	return lhs | Value(rhs);
}

Value icinga::operator|(double lhs, const Value& rhs)
{
	return Value(lhs) | rhs;
}

Value icinga::operator|(const Value& lhs, int rhs)
{
	return lhs | Value(rhs);
}

Value icinga::operator|(int lhs, const Value& rhs)
{
	return Value(lhs) | rhs;
}

Value icinga::operator<<(const Value& lhs, const Value& rhs)
{
	if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<int>(lhs) << static_cast<int>(rhs);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator << cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

Value icinga::operator<<(const Value& lhs, double rhs)
{
	return lhs << Value(rhs);
}

Value icinga::operator<<(double lhs, const Value& rhs)
{
	return Value(lhs) << rhs;
}

Value icinga::operator<<(const Value& lhs, int rhs)
{
	return lhs << Value(rhs);
}

Value icinga::operator<<(int lhs, const Value& rhs)
{
	return Value(lhs) << rhs;
}

Value icinga::operator>>(const Value& lhs, const Value& rhs)
{
	if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<int>(lhs) >> static_cast<int>(rhs);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator >> cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

Value icinga::operator>>(const Value& lhs, double rhs)
{
	return lhs >> Value(rhs);
}

Value icinga::operator>>(double lhs, const Value& rhs)
{
	return Value(lhs) >> rhs;
}

Value icinga::operator>>(const Value& lhs, int rhs)
{
	return lhs >> Value(rhs);
}

Value icinga::operator>>(int lhs, const Value& rhs)
{
	return Value(lhs) >> rhs;
}

bool icinga::operator<(const Value& lhs, const Value& rhs)
{
	if (lhs.IsString() && rhs.IsString())
		return static_cast<String>(lhs) < static_cast<String>(rhs);
	else if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<double>(lhs) < static_cast<double>(rhs);
	else if ((lhs.IsObjectType<DateTime>() || lhs.IsEmpty()) && (rhs.IsObjectType<DateTime>() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return Convert::ToDateTimeValue(lhs) < Convert::ToDateTimeValue(rhs);
	else if (lhs.IsObjectType<Array>() && rhs.IsObjectType<Array>()) {
		Array::Ptr larr = lhs;
		Array::Ptr rarr = rhs;

		ObjectLock llock(larr);
		ObjectLock rlock(rarr);

		Array::SizeType llen = larr->GetLength();
		Array::SizeType rlen = rarr->GetLength();

		for (Array::SizeType i = 0; i < std::max(llen, rlen); i++) {
			Value lval = (i >= llen) ? Empty : larr->Get(i);
			Value rval = (i >= rlen) ? Empty : rarr->Get(i);

			if (lval < rval)
				return true;
			else if (lval > rval)
				return false;
		}

		return false;
	} else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator < cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

bool icinga::operator<(const Value& lhs, double rhs)
{
	return lhs < Value(rhs);
}

bool icinga::operator<(double lhs, const Value& rhs)
{
	return Value(lhs) < rhs;
}

bool icinga::operator<(const Value& lhs, int rhs)
{
	return lhs < Value(rhs);
}

bool icinga::operator<(int lhs, const Value& rhs)
{
	return Value(lhs) < rhs;
}

bool icinga::operator>(const Value& lhs, const Value& rhs)
{
	if (lhs.IsString() && rhs.IsString())
		return static_cast<String>(lhs) > static_cast<String>(rhs);
	else if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<double>(lhs) > static_cast<double>(rhs);
	else if ((lhs.IsObjectType<DateTime>() || lhs.IsEmpty()) && (rhs.IsObjectType<DateTime>() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return Convert::ToDateTimeValue(lhs) > Convert::ToDateTimeValue(rhs);
	else if (lhs.IsObjectType<Array>() && rhs.IsObjectType<Array>()) {
		Array::Ptr larr = lhs;
		Array::Ptr rarr = rhs;

		ObjectLock llock(larr);
		ObjectLock rlock(rarr);

		Array::SizeType llen = larr->GetLength();
		Array::SizeType rlen = rarr->GetLength();

		for (Array::SizeType i = 0; i < std::max(llen, rlen); i++) {
			Value lval = (i >= llen) ? Empty : larr->Get(i);
			Value rval = (i >= rlen) ? Empty : rarr->Get(i);

			if (lval > rval)
				return true;
			else if (lval < rval)
				return false;
		}

		return false;
	} else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator > cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

bool icinga::operator>(const Value& lhs, double rhs)
{
	return lhs > Value(rhs);
}

bool icinga::operator>(double lhs, const Value& rhs)
{
	return Value(lhs) > rhs;
}

bool icinga::operator>(const Value& lhs, int rhs)
{
	return lhs > Value(rhs);
}

bool icinga::operator>(int lhs, const Value& rhs)
{
	return Value(lhs) > rhs;
}

bool icinga::operator<=(const Value& lhs, const Value& rhs)
{
	if (lhs.IsString() && rhs.IsString())
		return static_cast<String>(lhs) <= static_cast<String>(rhs);
	else if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<double>(lhs) <= static_cast<double>(rhs);
	else if ((lhs.IsObjectType<DateTime>() || lhs.IsEmpty()) && (rhs.IsObjectType<DateTime>() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return Convert::ToDateTimeValue(lhs) <= Convert::ToDateTimeValue(rhs);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator <= cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

bool icinga::operator<=(const Value& lhs, double rhs)
{
	return lhs <= Value(rhs);
}

bool icinga::operator<=(double lhs, const Value& rhs)
{
	return Value(lhs) <= rhs;
}

bool icinga::operator<=(const Value& lhs, int rhs)
{
	return lhs <= Value(rhs);
}

bool icinga::operator<=(int lhs, const Value& rhs)
{
	return Value(lhs) <= rhs;
}

bool icinga::operator>=(const Value& lhs, const Value& rhs)
{
	if (lhs.IsString() && rhs.IsString())
		return static_cast<String>(lhs) >= static_cast<String>(rhs);
	else if ((lhs.IsNumber() || lhs.IsEmpty()) && (rhs.IsNumber() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return static_cast<double>(lhs) >= static_cast<double>(rhs);
	else if ((lhs.IsObjectType<DateTime>() || lhs.IsEmpty()) && (rhs.IsObjectType<DateTime>() || rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()) && !(lhs.IsEmpty() && rhs.IsEmpty()))
		return Convert::ToDateTimeValue(lhs) >= Convert::ToDateTimeValue(rhs);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Operator >= cannot be applied to values of type '" + lhs.GetTypeName() + "' and '" + rhs.GetTypeName() + "'"));
}

bool icinga::operator>=(const Value& lhs, double rhs)
{
	return lhs >= Value(rhs);
}

bool icinga::operator>=(double lhs, const Value& rhs)
{
	return Value(lhs) >= rhs;
}

bool icinga::operator>=(const Value& lhs, int rhs)
{
	return lhs >= Value(rhs);
}

bool icinga::operator>=(int lhs, const Value& rhs)
{
	return Value(lhs) >= rhs;
}
