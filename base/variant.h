/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef VARIANT_H
#define VARIANT_H

struct cJSON;

namespace icinga
{

/**
 * A type that can hold an arbitrary value.
 *
 * @ingroup base
 */
class I2_BASE_API Variant
{
public:
	inline Variant(void)
		: m_Value()
	{ }

	inline Variant(int value)
		: m_Value(static_cast<long>(value))
	{ }

	inline Variant(long value)
		: m_Value(value)
	{ }

	inline Variant(double value)
		: m_Value(value)
	{ }

	inline Variant(bool value)
		: m_Value(static_cast<long>(value))
	{ }

	inline Variant(const string& value)
		: m_Value(value)
	{ }

	inline Variant(const char *value)
		: m_Value(string(value))
	{ }

	template<typename T>
	inline Variant(const shared_ptr<T>& value)
	{
		Object::Ptr object = dynamic_pointer_cast<Object>(value);

		if (!object)
			throw_exception(invalid_argument("shared_ptr value type must inherit from Object class."));

		m_Value = object;
	}

	operator long(void) const
	{
		if (m_Value.type() != typeid(long)) {
			return boost::lexical_cast<long>(m_Value);
		} else {
			return boost::get<long>(m_Value);
		}
	}

	operator double(void) const
	{
		if (m_Value.type() != typeid(double)) {
			return boost::lexical_cast<double>(m_Value);
		} else {
			return boost::get<double>(m_Value);
		}
	}

	operator bool(void) const
	{
		return (static_cast<long>(*this) != 0);
	}

	operator string(void) const
	{
		if (m_Value.type() != typeid(string)) {
			string result = boost::lexical_cast<string>(m_Value);
			m_Value = result;
			return result;
		} else {
			return boost::get<string>(m_Value);
		}
	}

	template<typename T>
	operator shared_ptr<T>(void) const
	{
		shared_ptr<T> object = dynamic_pointer_cast<T>(boost::get<Object::Ptr>(m_Value));

		if (!object)
			throw_exception(bad_cast());

		return object;
	}

	bool IsEmpty(void) const;
	bool IsScalar(void) const;
	bool IsObject(void) const;

	template<typename T>
	bool IsObjectType(void) const
	{
		if (!IsObject())
			return false;

		return (dynamic_pointer_cast<T>(boost::get<Object::Ptr>(m_Value)));
	}

	static Variant FromJson(cJSON *json);
	cJSON *ToJson(void) const;

	string Serialize(void) const;
	static Variant Deserialize(const string& jsonString);

private:
	mutable boost::variant<boost::blank, long, double, string, Object::Ptr> m_Value;
};

}

#endif /* VARIANT_H */
