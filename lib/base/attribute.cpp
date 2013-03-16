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

#include "base/attribute.h"
#include "base/utility.h"

using namespace icinga;

boost::mutex AttributeBase::m_Mutex;

AttributeBase::AttributeBase(void)
	: m_Value()
{ }

/**
 * @threadsafety Always.
 */
void AttributeBase::Set(const Value& value)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	InternalSet(value);
}

/**
 * @threadsafety Always.
 */
Value AttributeBase::Get(void) const
{
	boost::mutex::scoped_lock lock(m_Mutex);
	return InternalGet();
}

/**
 * @threadsafety Always.
 */
AttributeBase::operator Value(void) const
{
	boost::mutex::scoped_lock lock(m_Mutex);
	return InternalGet();
}

/**
 * @threadsafety Always.
 */
bool AttributeBase::IsEmpty(void) const
{
	boost::mutex::scoped_lock lock(m_Mutex);
	return InternalGet().IsEmpty();
}

/**
 * @threadsafety Caller must hold m_Mutex;
 */
void AttributeBase::InternalSet(const Value& value)
{
	m_Value = value;
}

/**
 * @threadsafety Caller must hold m_Mutex.
 */
const Value& AttributeBase::InternalGet(void) const
{
	return m_Value;
}

AttributeHolder::AttributeHolder(AttributeType type, AttributeBase *boundAttribute)
	: m_Type(type), m_Tx(0)
{
	if (boundAttribute) {
		m_Attribute = boundAttribute;
		m_OwnsAttribute = false;
	} else {
		m_Attribute = new Attribute<Value>();
		m_OwnsAttribute = true;
	}
}

AttributeHolder::AttributeHolder(const AttributeHolder& other)
{
	m_Type = other.m_Type;
	m_Tx = other.m_Tx;
	m_OwnsAttribute = other.m_OwnsAttribute;

	if (other.m_OwnsAttribute) {
		m_Attribute = new Attribute<Value>();
		m_Attribute->Set(other.m_Attribute->Get());
	} else {
		m_Attribute = other.m_Attribute;
	}
}

AttributeHolder::~AttributeHolder(void)
{
	if (m_OwnsAttribute)
		delete m_Attribute;
}

void AttributeHolder::Bind(AttributeBase *boundAttribute)
{
	ASSERT(m_OwnsAttribute);
	boundAttribute->Set(m_Attribute->Get());
	m_Attribute = boundAttribute;
	m_OwnsAttribute = false;
}

void AttributeHolder::SetValue(double tx, const Value& value)
{
	m_Tx = tx;
	m_Attribute->Set(value);
}

Value AttributeHolder::GetValue(void) const
{
	return m_Attribute->Get();
}

void AttributeHolder::SetType(AttributeType type)
{
	m_Type = type;
}

AttributeType AttributeHolder::GetType(void) const
{
	return m_Type;
}

void AttributeHolder::SetTx(double tx)
{
	m_Tx = tx;
}

double AttributeHolder::GetTx(void) const
{
	return m_Tx;
}
