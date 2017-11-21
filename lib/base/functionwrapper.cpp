/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/functionwrapper.hpp"

using namespace icinga;

Value icinga::FunctionWrapperVV(void (*function)(void), const std::vector<Value>&)
{
	function();

	return Empty;
}

Value icinga::FunctionWrapperVA(void (*function)(const std::vector<Value>&), const std::vector<Value>& arguments)
{
	function(arguments);

	return Empty;
}

std::function<Value (const std::vector<Value>& arguments)> icinga::WrapFunction(void (*function)(void))
{
	return std::bind(&FunctionWrapperVV, function, _1);
}
