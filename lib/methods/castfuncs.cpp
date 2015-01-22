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

#include "methods/castfuncs.hpp"
#include "base/scriptfunction.hpp"

using namespace icinga;

REGISTER_SCRIPTFUNCTION(string, &CastFuncs::CastString);
REGISTER_SCRIPTFUNCTION(number, &CastFuncs::CastNumber);
REGISTER_SCRIPTFUNCTION(bool, &CastFuncs::CastBool);

String CastFuncs::CastString(const Value& value)
{
	return value;
}

double CastFuncs::CastNumber(const Value& value)
{
	return value;
}

bool CastFuncs::CastBool(const Value& value)
{
	return value.ToBool();
}
