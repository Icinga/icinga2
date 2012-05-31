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

#ifndef DYNAMICDICTIONARY_H
#define DYNAMICDICTIONARY_H

namespace icinga
{

enum DynamicDictionaryOperator
{
	OperatorSet,
	OperatorPlus,
	OperatorMinus,
	OperatorMultiply,
	OperatorDivide
};

struct DynamicDictionaryValue
{
	Variant Value;
	DynamicDictionaryOperator Operator;
};

class I2_DYN_API DynamicDictionary : public Object
{
public:
	typedef shared_ptr<DynamicDictionary> Ptr;
	typedef weak_ptr<DynamicDictionary> WeakPtr;

	DynamicDictionary(void);
//	DynamicDictionary(Dictionary::Ptr serializedDictionary);

//	void AddParent(DynamicDictionary::Ptr parent);
//	void ClearParents(void);

	template<typename T>
	bool GetProperty(string name, T *value, DynamicDictionaryOperator *op) const
	{
		map<string, DynamicDictionaryValue>::const_iterator di;

		di = m_Values.find(name);
		if (di == m_Values.end())
			return false;

		*value = di->second.Value;
		*op = di->second.Operator;
		return true;
	}

	template<typename T>
	void SetProperty(string name, const T& value, DynamicDictionaryOperator op)
	{
		DynamicDictionaryValue ddv;
		ddv.Value = value;
		ddv.Operator = op;
		m_Values[name] = ddv;
	}

//	Dictionary::Ptr ToFlatDictionary(void) const;

//	Dictionary::Ptr Serialize(void);

private:
//	set<DynamicDictionary::Ptr> m_Parents;
	map<string, DynamicDictionaryValue> m_Values;
};

}

#endif /* DYNAMICDICTIONARY_H */
