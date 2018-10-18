/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
#include <boost/function_types/function_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/function_types/function_arity.hpp>
#include <vector>

using namespace std::placeholders;

namespace icinga
{

template<typename FuncType>
typename std::enable_if<
    std::is_class<FuncType>::value &&
    std::is_same<typename boost::function_types::result_type<decltype(&FuncType::operator())>::type, Value>::value &&
	boost::function_types::function_arity<decltype(&FuncType::operator())>::value == 2,
    std::function<Value (const std::vector<Value>&)>>::type
WrapFunction(FuncType function)
{
	static_assert(std::is_same<typename boost::mpl::at_c<typename boost::function_types::parameter_types<decltype(&FuncType::operator())>, 1>::type, const std::vector<Value>&>::value, "Argument type must be const std::vector<Value>");
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
	return std::bind(function, _1);
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

struct UnpackCaller
{
private:
	template <typename FuncType, size_t... I>
	auto Invoke(FuncType f, const std::vector<Value>& args, indices<I...>) -> decltype(f(args[I]...))
	{
		return f(args[I]...);
	}

public:
	template <typename FuncType, int Arity>
	auto operator() (FuncType f, const std::vector<Value>& args) -> decltype(Invoke(f, args, BuildIndices<Arity>{}))
	{
		return Invoke(f, args, BuildIndices<Arity>{});
	}
};

template<typename FuncType, int Arity, typename ReturnType>
struct FunctionWrapper
{
	static Value Invoke(FuncType function, const std::vector<Value>& arguments)
	{
		return UnpackCaller().operator()<FuncType, Arity>(function, arguments);
	}
};

template<typename FuncType, int Arity>
struct FunctionWrapper<FuncType, Arity, void>
{
	static Value Invoke(FuncType function, const std::vector<Value>& arguments)
	{
		UnpackCaller().operator()<FuncType, Arity>(function, arguments);
		return Empty;
	}
};

template<typename FuncType>
typename std::enable_if<
	std::is_function<typename std::remove_pointer<FuncType>::type>::value && !std::is_same<FuncType, Value(*)(const std::vector<Value>&)>::value,
	std::function<Value (const std::vector<Value>&)>>::type
WrapFunction(FuncType function)
{
	return [function](const std::vector<Value>& arguments) {
		constexpr size_t arity = boost::function_types::function_arity<typename std::remove_pointer<FuncType>::type>::value;

		if (arity > 0) {
			if (arguments.size() < arity)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
			else if (arguments.size() > arity)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));
		}

		using ReturnType = decltype(UnpackCaller().operator()<FuncType, arity>(*static_cast<FuncType *>(nullptr), std::vector<Value>()));

		return FunctionWrapper<FuncType, arity, ReturnType>::Invoke(function, arguments);
	};
}

template<typename FuncType>
typename std::enable_if<
    std::is_class<FuncType>::value &&
    !(std::is_same<typename boost::function_types::result_type<decltype(&FuncType::operator())>::type, Value>::value &&
	boost::function_types::function_arity<decltype(&FuncType::operator())>::value == 2),
    std::function<Value (const std::vector<Value>&)>>::type
WrapFunction(FuncType function)
{
	static_assert(!std::is_same<typename boost::mpl::at_c<typename boost::function_types::parameter_types<decltype(&FuncType::operator())>, 1>::type, const std::vector<Value>&>::value, "Argument type must be const std::vector<Value>");

	using FuncTypeInvoker = decltype(&FuncType::operator());

	return [function](const std::vector<Value>& arguments) {
		constexpr size_t arity = boost::function_types::function_arity<FuncTypeInvoker>::value - 1;

		if (arity > 0) {
			if (arguments.size() < arity)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function."));
			else if (arguments.size() > arity)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Too many arguments for function."));
		}

		using ReturnType = decltype(UnpackCaller().operator()<FuncType, arity>(*static_cast<FuncType *>(nullptr), std::vector<Value>()));

		return FunctionWrapper<FuncType, arity, ReturnType>::Invoke(function, arguments);
	};
}

}

#endif /* FUNCTIONWRAPPER_H */
