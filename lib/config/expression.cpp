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

#include "i2-config.h"

using namespace icinga;

Expression::Expression(const String& key, ExpressionOperator op,
    const Value& value, const DebugInfo& debuginfo)
	: m_Key(key), m_Operator(op), m_Value(value), m_DebugInfo(debuginfo)
{
}

void Expression::Execute(const Dictionary::Ptr& dictionary) const
{
	Value oldValue, newValue;

	ExpressionList::Ptr valueExprl;
	Dictionary::Ptr valueDict;
	if (m_Value.IsObjectType<ExpressionList>())
		valueExprl = m_Value;

	if (m_Value.IsObjectType<Dictionary>())
		valueDict = m_Value;

	newValue = m_Value;

	Dictionary::Ptr dict;

	switch (m_Operator) {
		case OperatorExecute:
			if (!valueExprl)
				BOOST_THROW_EXCEPTION(invalid_argument("Operand for OperatorExecute must be an ExpressionList."));

			valueExprl->Execute(dictionary);

			return;

		case OperatorSet:
			if (valueExprl) {
				ObjectLock olock(valueExprl);
				dict = boost::make_shared<Dictionary>();
				valueExprl->Execute(dict);
				newValue = dict;
			}

			break;

		case OperatorPlus:
			{
				ObjectLock olock(dictionary);
				oldValue = dictionary->Get(m_Key);
			}

			if (oldValue.IsObjectType<Dictionary>())
				dict = oldValue;

			if (!dict) {
				if (!oldValue.IsEmpty()) {
					stringstream message;
					message << "Wrong argument types for"
					    " += (non-dictionary and"
					    " dictionary) ("
					        << m_DebugInfo << ")";
					BOOST_THROW_EXCEPTION(domain_error(message.str()));
				}

				dict = boost::make_shared<Dictionary>();
			}

			newValue = dict;

			if (valueExprl) {
				ObjectLock olock(valueExprl);

				valueExprl->Execute(dict);
			} else if (valueDict) {
				ObjectLock olock(valueDict);
				ObjectLock dlock(dict);

				String key;
				Value value;
				BOOST_FOREACH(tie(key, value), valueDict) {
					dict->Set(key, value);
				}
			} else {
				stringstream message;
				message << "+= only works for dictionaries ("
					<< m_DebugInfo << ")";
				BOOST_THROW_EXCEPTION(domain_error(message.str()));
			}

			break;

		default:
			BOOST_THROW_EXCEPTION(runtime_error("Not yet implemented."));
	}

	ObjectLock olock(dictionary);
	if (m_Key.IsEmpty())
		dictionary->Add(newValue);
	else
		dictionary->Set(m_Key, newValue);
}

void Expression::DumpValue(ostream& fp, int indent, const Value& value, bool inlineDict)
{
	ExpressionList::Ptr valueExprl;
	Dictionary::Ptr valueDict;
	if (value.IsObjectType<ExpressionList>()) {
		if (!inlineDict)
			fp << "{ " << "\n";

		static_cast<ExpressionList::Ptr>(value)->Dump(fp, indent);

		if (!inlineDict) {
			for (int i = 0; i < indent - 1; i++)
				fp << "\t";

			fp << "}";
		}

		return;
	}

	if (value.IsObjectType<Dictionary>()) {
		if (!inlineDict)
			fp << "{ " << "\n";

		String k;
		Value v;
		BOOST_FOREACH(tie(k, v), static_cast<Dictionary::Ptr>(value)) {
			for (int i = 0; i < indent; i++)
				fp << "\t";

			fp << "\"" << k << "\" = ";
			DumpValue(fp, indent, v);
			fp << "," << "\n";

			fp << "}";
		}

		if (!inlineDict)
			fp << "}";

		return;
	}

	if (value.IsScalar()) {
		fp << "\"" << static_cast<String>(value) << "\"";
		return;
	}

	BOOST_THROW_EXCEPTION(runtime_error("Encountered unknown type while dumping value."));
}

/**
 * @threadsafety Always.
 */
void Expression::Dump(ostream& fp, int indent) const
{
	if (m_Operator == OperatorExecute) {
		DumpValue(fp, indent, m_Value, true);
		return;
	}

	for (int i = 0; i < indent; i++)
		fp << "\t";

	fp << "\"" << m_Key << "\" ";

	switch (m_Operator) {
		case OperatorSet:
			fp << "=";
			break;
		case OperatorPlus:
			fp << "+=";
			break;
		default:
			BOOST_THROW_EXCEPTION(runtime_error("Not yet implemented."));
	}

	fp << " ";

	DumpValue(fp, indent + 1, m_Value);

	fp << ", " << "\n";
}
