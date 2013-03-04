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

#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

namespace icinga
{

/**
 * The type of an attribute for a DynamicObject.
 *
 * @ingroup base
 */
enum AttributeType
{
	Attribute_Transient = 1,

	/* Unlike transient attributes local attributes are persisted
	 * in the program state file. */
	Attribute_Local = 2,

	/* Replicated attributes are sent to other daemons for which
	 * replication is enabled. */
	Attribute_Replicated = 4,

	/* Attributes read from the config file are implicitly marked
	 * as config attributes. */
	Attribute_Config = 8,

	/* Combination of all attribute types */
	Attribute_All = Attribute_Transient | Attribute_Local | Attribute_Replicated | Attribute_Config
};

class I2_BASE_API AttributeBase
{
public:
	AttributeBase(void);

	void Set(const Value& value);
	Value Get(void) const;
	operator Value(void) const;
	bool IsEmpty(void) const;

protected:
	void InternalSet(const Value& value);
	const Value& InternalGet(void) const;

	static boost::mutex m_Mutex;

private:
	Value m_Value;

	AttributeBase(const AttributeBase& other);
	AttributeBase& operator=(const AttributeBase& other);
};

template<typename T>
class Attribute : public AttributeBase
{
public:
	/**
	 * @threadsafety Always.
	 */
	void Set(const T& value)
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		InternalSet(value);
	}

	/**
	 * @threadsafety Always.
	 */
	Attribute<T>& operator=(const T& rhs)
	{
		Set(rhs);
		return *this;
	}

	T Get(void) const
	{
		Value value;

		{
			boost::mutex::scoped_lock lock(m_Mutex);
			value = InternalGet();
		}

		if (value.IsEmpty())
			return T();

		return value;
	}

	/**
	 * @threadsafety Always.
	 */
	operator T(void) const
	{
		return Get();
	}
};

/**
 * An attribute for a DynamicObject.
 *
 * @ingroup base
 */
class I2_BASE_API AttributeHolder
{
public:
	AttributeHolder(AttributeType type, AttributeBase *boundAttribute = NULL);
	AttributeHolder(const AttributeHolder& other);
	~AttributeHolder(void);

	void Bind(AttributeBase *boundAttribute);

	void SetValue(double tx, const Value& value);
	Value GetValue(void) const;

	void SetType(AttributeType type);
	AttributeType GetType(void) const;

	void SetTx(double tx);
	double GetTx(void) const;

private:
	AttributeType m_Type; /**< The type of the attribute. */
	double m_Tx; /**< The timestamp of the last value change. */
	bool m_OwnsAttribute; /**< Whether we own the Data pointer. */
	AttributeBase *m_Attribute; /**< The current value of the attribute. */
};

}

#endif /* ATTRIBUTE_H */
