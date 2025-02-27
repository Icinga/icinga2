/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/value.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/type.hpp"

using namespace icinga;

template class boost::variant<boost::blank, double, bool, String, Object::Ptr>;
template const double& Value::Get<double>() const;
template const bool& Value::Get<bool>() const;
template const String& Value::Get<String>() const;
template String&& Value::Get<String>();
template const Object::Ptr& Value::Get<Object::Ptr>() const;
template Object::Ptr&& Value::Get<Object::Ptr>();

const Value icinga::Empty;

Value::Value(std::nullptr_t)
{ }

Value::Value(int value)
	: m_Value(double(value))
{ }

Value::Value(unsigned int value)
	: m_Value(double(value))
{ }

Value::Value(long value)
	: m_Value(double(value))
{ }

Value::Value(unsigned long value)
	: m_Value(double(value))
{ }

Value::Value(long long value)
	: m_Value(double(value))
{ }

Value::Value(unsigned long long value)
	: m_Value(double(value))
{ }

Value::Value(double value)
	: m_Value(value)
{ }

Value::Value(bool value)
	: m_Value(value)
{ }

Value::Value(const String& value)
	: m_Value(value)
{ }

Value::Value(String&& value)
	: m_Value(value)
{ }

Value::Value(const char *value)
	: m_Value(String(value))
{ }

Value::Value(const Value& other)
	: m_Value(other.m_Value)
{ }

Value::Value(Value&& other)
{
#if BOOST_VERSION >= 105400
	m_Value = std::move(other.m_Value);
#else /* BOOST_VERSION */
	m_Value.swap(other.m_Value);
#endif /* BOOST_VERSION */
}

Value::Value(Object *value)
	: Value(Object::Ptr(value))
{ }

Value::Value(const intrusive_ptr<Object>& value)
{
	if (value)
		m_Value = value;
}

Value& Value::operator=(const Value& other)
{
	m_Value = other.m_Value;
	return *this;
}

Value& Value::operator=(Value&& other)
{
#if BOOST_VERSION >= 105400
	m_Value = std::move(other.m_Value);
#else /* BOOST_VERSION */
	m_Value.swap(other.m_Value);
#endif /* BOOST_VERSION */

	return *this;
}

/**
 * Checks whether the variant is empty.
 *
 * @returns true if the variant is empty, false otherwise.
 */
bool Value::IsEmpty() const
{
	return (GetType() == ValueEmpty || (IsString() && boost::get<String>(m_Value).IsEmpty()));
}

/**
 * Checks whether the variant is scalar (i.e. not an object and not empty).
 *
 * @returns true if the variant is scalar, false otherwise.
 */
bool Value::IsScalar() const
{
	return !IsEmpty() && !IsObject();
}

/**
* Checks whether the variant is a number.
*
* @returns true if the variant is a number.
*/
bool Value::IsNumber() const
{
	return (GetType() == ValueNumber);
}

/**
 * Checks whether the variant is a boolean.
 *
 * @returns true if the variant is a boolean.
 */
bool Value::IsBoolean() const
{
	return (GetType() == ValueBoolean);
}

/**
 * Checks whether the variant is a string.
 *
 * @returns true if the variant is a string.
 */
bool Value::IsString() const
{
	return (GetType() == ValueString);
}

/**
 * Checks whether the variant is a non-null object.
 *
 * @returns true if the variant is a non-null object, false otherwise.
 */
bool Value::IsObject() const
{
	return  (GetType() == ValueObject);
}

/**
 * Returns the type of the value.
 *
 * @returns The type.
 */
ValueType Value::GetType() const
{
	return static_cast<ValueType>(m_Value.which());
}

void Value::Swap(Value& other)
{
	m_Value.swap(other.m_Value);
}

bool Value::ToBool() const
{
	switch (GetType()) {
		case ValueNumber:
			return static_cast<bool>(boost::get<double>(m_Value));

		case ValueBoolean:
			return boost::get<bool>(m_Value);

		case ValueString:
			return !boost::get<String>(m_Value).IsEmpty();

		case ValueObject:
			if (IsObjectType<Dictionary>()) {
				Dictionary::Ptr dictionary = *this;
				return dictionary->GetLength() > 0;
			} else if (IsObjectType<Array>()) {
				Array::Ptr array = *this;
				return array->GetLength() > 0;
			} else {
				return true;
			}

		case ValueEmpty:
			return false;

		default:
			BOOST_THROW_EXCEPTION(std::runtime_error("Invalid variant type."));
	}
}

String Value::GetTypeName() const
{
	Type::Ptr t;

	switch (GetType()) {
		case ValueEmpty:
			return "Empty";
		case ValueNumber:
			return "Number";
		case ValueBoolean:
			return "Boolean";
		case ValueString:
			return "String";
		case ValueObject:
			t = boost::get<Object::Ptr>(m_Value)->GetReflectionType();
			if (!t) {
				if (IsObjectType<Array>())
					return "Array";
				else if (IsObjectType<Dictionary>())
					return "Dictionary";
				else
					return "Object";
			} else
				return t->GetName();
		default:
			return "Invalid";
	}
}

Type::Ptr Value::GetReflectionType() const
{
	switch (GetType()) {
		case ValueEmpty:
			return Object::TypeInstance;
		case ValueNumber:
			return Type::GetByName("Number");
		case ValueBoolean:
			return Type::GetByName("Boolean");
		case ValueString:
			return Type::GetByName("String");
		case ValueObject:
			return boost::get<Object::Ptr>(m_Value)->GetReflectionType();
		default:
			return nullptr;
	}
}

Value Value::Clone() const
{
	if (IsObject())
		return static_cast<Object::Ptr>(*this)->Clone();
	else
		return *this;
}
