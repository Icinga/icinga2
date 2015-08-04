/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/customvarobject.hpp"
#include "icinga/customvarobject.tcpp"
#include "icinga/macroprocessor.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/exception.hpp"
#include "base/objectlock.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(CustomVarObject);

int CustomVarObject::GetModifiedAttributes(void) const
{
	//TODO-MA
	return 0;
}

void CustomVarObject::SetModifiedAttributes(int, const MessageOrigin::Ptr&)
{
	//TODO-MA
}

bool CustomVarObject::IsVarOverridden(const String& name) const
{
	//TODO: implement
	return false;
}

void CustomVarObject::ValidateVars(const Dictionary::Ptr& value, const ValidationUtils& utils)
{
	if (!value)
		return;

	/* string, array, dictionary */
	ObjectLock olock(value);
	BOOST_FOREACH(const Dictionary::Pair& kv, value) {
		const Value& varval = kv.second;

		if (varval.IsObjectType<Dictionary>()) {
			/* only one dictonary level */
			Dictionary::Ptr varval_dict = varval;

			ObjectLock xlock(varval_dict);
			BOOST_FOREACH(const Dictionary::Pair& kv_var, varval_dict) {
				if (kv_var.second.IsEmpty())
					continue;

				if (!MacroProcessor::ValidateMacroString(kv_var.second))
					BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("vars")(kv.first)(kv_var.first), "Closing $ not found in macro format string '" + kv_var.second + "'."));
			}
		} else if (varval.IsObjectType<Array>()) {
			/* check all array entries */
			Array::Ptr varval_arr = varval;

			ObjectLock ylock (varval_arr);
			BOOST_FOREACH(const Value& arrval, varval_arr) {
				if (arrval.IsEmpty())
					continue;

				if (!MacroProcessor::ValidateMacroString(arrval)) {
					BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("vars")(kv.first), "Closing $ not found in macro format string '" + arrval + "'."));
				}
			}
		} else {
			if (varval.IsEmpty())
				continue;

			String varstr = varval;

			if (!MacroProcessor::ValidateMacroString(varstr))
				BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("vars")(kv.first), "Closing $ not found in macro format string '" + varstr + "'."));
		}
	}
}
