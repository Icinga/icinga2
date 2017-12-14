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

#ifndef FUNCTIONWRAPPER_H
#define FUNCTIONWRAPPER_H

#include "base/i2-base.hpp"
#include "base/value.hpp"
#include <vector>
#include <boost/function_types/function_arity.hpp>
#include <type_traits>

using namespace std::placeholders;

namespace icinga
{

inline std::function<Value (const std::vector<Value>&)> WrapFunction(const std::function<Value(const std::vector<Value>&)>& function)
{
	return function;
}

inline std::function<Value (const std::vector<Value>&)> WrapFunction(void (*function)(const std::vector<Value>&))
{
	return [function](const std::vector<Value>& arguments) {
		function(arguments);
		return Empty;
	};
}
template<typename Return>
std::function<Value (const std::vector<Value>&)> WrapFunction(Return (*function)(const std::vector<Value>&))
{
	return [function](const std::vector<Value>& arguments) {
		return static_cast<Value>(function(arguments));
	};
}

template <std::size_t... Indices>
struct indices {
	using next = indices<Indices..., sizeof...(Indices)>;
};

template <std::size_t N>
struct build_indices {
	using type = typename build_indices<N-1>::type::next;
};

template <>
struct build_indices<0> {
	using type = indices<>;
};

template <std::size_t N>
using BuildIndices = typename build_indices<N>::type;

struct unpack_caller
{
private:
	template <typename FuncType, size_t... I>
	auto call(FuncType f, const std::vector<Value>& args, indices<I...>) -> decltype(f(args[I]...))
	{
		return f(args[I]...);
	}

public:
	template <typename FuncType>
	auto operator () (FuncType f, const std::vector<Value>& args)
	    -> decltype(call(f, args, BuildIndices<boost::function_types::function_arity<typename boost::remove_pointer<FuncType>::type>::value>{}))
	{
		return call(f, args, BuildIndices<boost::function_types::function_arity<typename boost::remove_pointer<FuncType>::type>::value>{});
	}
};

enum class enabler_t {};

template<bool T>
using EnableIf = typename std::enable_if<T, enabler_t>::type;

template<typename FuncType>
std::function<Value (const std::vector<Value>&)> WrapFunction(FuncType function,
    EnableIf<std::is_same<decltype(unpack_caller()(FuncType(), std::vector<Value>())), void>::value>* = 0)
{
	return [function](const std::vector<Value>& arguments) {
		constexpr int arity = boost::function_types::function_arity<typename boost::remove_pointer<FuncType>::type>::value;

		if (arguments.size() < arity)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
		else if (arguments.size() > arity)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));

		unpack_caller()(function, arguments);
		return Empty;
	};
}

template<typename FuncType>
std::function<Value (const std::vector<Value>&)> WrapFunction(FuncType function,
		EnableIf<!std::is_same<decltype(unpack_caller()(FuncType(), std::vector<Value>())), void>::value>* = 0)
{
	return [function](const std::vector<Value>& arguments) {
		constexpr int arity = boost::function_types::function_arity<typename boost::remove_pointer<FuncType>::type>::value;

		if (arity > 0) {
			if (arguments.size() < arity)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
			else if (arguments.size() > arity)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));
		}

		return unpack_caller()(function, arguments);
	};
}

}

#endif /* FUNCTIONWRAPPER_H */
