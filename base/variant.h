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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef VARIANT_H
#define VARIANT_H

namespace icinga
{

enum I2_BASE_API VariantType
{
	VariantEmpty,
	VariantInteger,
	VariantString,
	VariantObject
};

class I2_BASE_API Variant
{
private:
	mutable long m_IntegerValue;
	mutable string m_StringValue;
	mutable Object::Ptr m_ObjectValue;

	mutable VariantType m_Type;

	void Convert(VariantType newType) const;

public:
	inline Variant(void) : m_Type(VariantEmpty) { }

	inline Variant(long value) : m_Type(VariantInteger), m_IntegerValue(value) { }

	inline Variant(const char *value) : m_Type(VariantString), m_StringValue(string(value)) { }

	inline Variant(string value) : m_Type(VariantString), m_StringValue(value) { }

	template<typename T>
	Variant(const shared_ptr<T>& value) : m_Type(VariantObject), m_ObjectValue(value) { }

	VariantType GetType(void) const;

	long GetInteger(void) const;
	string GetString(void) const;
	Object::Ptr GetObject(void) const;

	bool IsEmpty(void) const;

	operator long(void) const;
	operator string(void) const;
	operator Object::Ptr(void) const;
};

}

#endif /* VARIANT_H */
