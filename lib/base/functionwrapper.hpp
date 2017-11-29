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

#ifndef SCRIPTFUNCTIONWRAPPER_H
#define SCRIPTFUNCTIONWRAPPER_H

#include "base/i2-base.hpp"
#include "base/value.hpp"
#include <vector>

using namespace std::placeholders;

namespace icinga
{

Value FunctionWrapperVV(void (*function)(void), const std::vector<Value>& arguments);
Value FunctionWrapperVA(void (*function)(const std::vector<Value>&), const std::vector<Value>& arguments);

std::function<Value (const std::vector<Value>& arguments)> I2_BASE_API WrapFunction(void (*function)(void));

template<typename TR>
Value FunctionWrapperR(TR (*function)(void), const std::vector<Value>&)
{
	return function();
}

template<typename TR>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(void))
{
	return std::bind(&FunctionWrapperR<TR>, function, _1);
}

template<typename T0>
Value FunctionWrapperV(void (*function)(T0), const std::vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	function(static_cast<T0>(arguments[0]));

	return Empty;
}

template<typename T0>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(void (*function)(T0))
{
	return std::bind(&FunctionWrapperV<T0>, function, _1);
}

template<typename TR, typename T0>
Value FunctionWrapperR(TR (*function)(T0), const std::vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	return function(static_cast<T0>(arguments[0]));
}

template<typename TR, typename T0>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(T0))
{
	return std::bind(&FunctionWrapperR<TR, T0>, function, _1);
}

template<typename T0, typename T1>
Value FunctionWrapperV(void (*function)(T0, T1), const std::vector<Value>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]));

	return Empty;
}

template<typename T0, typename T1>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(void (*function)(T0, T1))
{
	return std::bind(&FunctionWrapperV<T0, T1>, function, _1);
}

template<typename TR, typename T0, typename T1>
Value FunctionWrapperR(TR (*function)(T0, T1), const std::vector<Value>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	return function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]));
}

template<typename TR, typename T0, typename T1>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(T0, T1))
{
	return std::bind(&FunctionWrapperR<TR, T0, T1>, function, _1);
}

template<typename T0, typename T1, typename T2>
Value FunctionWrapperV(void (*function)(T0, T1, T2), const std::vector<Value>& arguments)
{
	if (arguments.size() < 3)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 3)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]));

	return Empty;
}

template<typename T0, typename T1, typename T2>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(void (*function)(T0, T1, T2))
{
	return std::bind(&FunctionWrapperV<T0, T1, T2>, function, _1);
}

template<typename TR, typename T0, typename T1, typename T2>
Value FunctionWrapperR(TR (*function)(T0, T1, T2), const std::vector<Value>& arguments)
{
	if (arguments.size() < 3)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 3)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	return function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]));
}

template<typename TR, typename T0, typename T1, typename T2>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(T0, T1, T2))
{
	return std::bind(&FunctionWrapperR<TR, T0, T1, T2>, function, _1);
}

template<typename T0, typename T1, typename T2, typename T3>
Value FunctionWrapperV(void (*function)(T0, T1, T2, T3), const std::vector<Value>& arguments)
{
	if (arguments.size() < 4)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 4)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]));

	return Empty;
}

template<typename T0, typename T1, typename T2, typename T3>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(void (*function)(T0, T1, T2, T3))
{
	return std::bind(&FunctionWrapperV<T0, T1, T2, T3>, function, _1);
}

template<typename TR, typename T0, typename T1, typename T2, typename T3>
Value FunctionWrapperR(TR (*function)(T0, T1, T2, T3), const std::vector<Value>& arguments)
{
	if (arguments.size() < 4)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 4)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	return function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]));
}

template<typename TR, typename T0, typename T1, typename T2, typename T3>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(T0, T1, T2, T3))
{
	return std::bind(&FunctionWrapperR<TR, T0, T1, T2, T3>, function, _1);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4>
Value FunctionWrapperV(void (*function)(T0, T1, T2, T3, T4), const std::vector<Value>& arguments)
{
	if (arguments.size() < 5)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 5)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]),
	    static_cast<T4>(arguments[4]));

	return Empty;
}

template<typename T0, typename T1, typename T2, typename T3, typename T4>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(void (*function)(T0, T1, T2, T3, T4))
{
	return std::bind(&FunctionWrapperV<T0, T1, T2, T3, T4>, function, _1);
}

template<typename TR, typename T0, typename T1, typename T2, typename T3, typename T4>
Value FunctionWrapperR(TR (*function)(T0, T1, T2, T3, T4), const std::vector<Value>& arguments)
{
	if (arguments.size() < 5)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 5)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	return function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]),
	    static_cast<T4>(arguments[4]));
}

template<typename TR, typename T0, typename T1, typename T2, typename T3, typename T4>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(T0, T1, T2, T3, T4))
{
	return std::bind(&FunctionWrapperR<TR, T0, T1, T2, T3, T4>, function, _1);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
Value FunctionWrapperV(void (*function)(T0, T1, T2, T3, T4, T5), const std::vector<Value>& arguments)
{
	if (arguments.size() < 6)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 6)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]),
	    static_cast<T4>(arguments[4]),
	    static_cast<T5>(arguments[5]));

	return Empty;
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(void (*function)(T0, T1, T2, T3, T4, T5))
{
	return std::bind(&FunctionWrapperV<T0, T1, T2, T3, T4, T5>, function, _1);
}

template<typename TR, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
Value FunctionWrapperR(TR (*function)(T0, T1, T2, T3, T4, T5), const std::vector<Value>& arguments)
{
	if (arguments.size() < 6)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 6)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	return function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]),
	    static_cast<T4>(arguments[4]),
	    static_cast<T5>(arguments[5]));
}

template<typename TR, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(T0, T1, T2, T3, T4, T5))
{
	return std::bind(&FunctionWrapperR<TR, T0, T1, T2, T3, T4, T5>, function, _1);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
Value FunctionWrapperV(void (*function)(T0, T1, T2, T3, T4, T5, T6), const std::vector<Value>& arguments)
{
	if (arguments.size() < 7)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 7)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]),
	    static_cast<T4>(arguments[4]),
	    static_cast<T5>(arguments[5]),
	    static_cast<T6>(arguments[6]));

	return Empty;
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(void (*function)(T0, T1, T2, T3, T4, T5, T6))
{
	return std::bind(&FunctionWrapperV<T0, T1, T2, T3, T4, T5, T6>, function, _1);
}

template<typename TR, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
Value FunctionWrapperR(TR (*function)(T0, T1, T2, T3, T4, T5, T6), const std::vector<Value>& arguments)
{
	if (arguments.size() < 7)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 7)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	return function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]),
	    static_cast<T4>(arguments[4]),
	    static_cast<T5>(arguments[5]),
	    static_cast<T6>(arguments[6]));
}

template<typename TR, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(T0, T1, T2, T3, T4, T5, T6))
{
	return std::bind(&FunctionWrapperR<TR, T0, T1, T2, T3, T4, T5, T6>, function, _1);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
Value FunctionWrapperV(void (*function)(T0, T1, T2, T3, T4, T5, T6, T7), const std::vector<Value>& arguments)
{
	if (arguments.size() < 8)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 8)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]),
	    static_cast<T4>(arguments[4]),
	    static_cast<T5>(arguments[5]),
	    static_cast<T6>(arguments[6]),
	    static_cast<T7>(arguments[7]));

	return Empty;
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(void (*function)(T0, T1, T2, T3, T4, T5, T6, T7))
{
	return std::bind(&FunctionWrapperV<T0, T1, T2, T3, T4, T5, T6, T7>, function, _1);
}

template<typename TR, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
Value FunctionWrapperR(TR (*function)(T0, T1, T2, T3, T4, T5, T6, T7), const std::vector<Value>& arguments)
{
	if (arguments.size() < 8)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
	else if (arguments.size() > 8)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

	return function(static_cast<T0>(arguments[0]),
	    static_cast<T1>(arguments[1]),
	    static_cast<T2>(arguments[2]),
	    static_cast<T3>(arguments[3]),
	    static_cast<T4>(arguments[4]),
	    static_cast<T5>(arguments[5]),
	    static_cast<T6>(arguments[6]),
	    static_cast<T7>(arguments[7]));
}

template<typename TR, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
std::function<Value (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(T0, T1, T2, T3, T4, T5, T6, T7))
{
	return std::bind(&FunctionWrapperR<TR, T0, T1, T2, T3, T4, T5, T6, T7>, function, _1);
}

template<typename TR>
std::function<TR (const std::vector<Value>& arguments)> WrapFunction(TR (*function)(const std::vector<Value>&))
{
	return std::bind<TR>(function, _1);
}

inline std::function<Value (const std::vector<Value>& arguments)> WrapFunction(void (*function)(const std::vector<Value>&))
{
	return std::bind(&FunctionWrapperVA, function, _1);
}

}

#endif /* SCRIPTFUNCTION_H */
