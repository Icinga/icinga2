/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/initialize.hpp"
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/foreach.hpp>
#include <cmath>

using namespace icinga;

static double MathAbs(double x)
{
	return std::fabs(x);
}

static double MathAcos(double x)
{
	return std::acos(x);
}

static double MathAsin(double x)
{
	return std::asin(x);
}

static double MathAtan(double x)
{
	return std::atan(x);
}

static double MathAtan2(double x, double y)
{
	return std::atan2(x, y);
}

static double MathCeil(double x)
{
	return std::ceil(x);
}

static double MathCos(double x)
{
	return std::cos(x);
}

static double MathExp(double x)
{
	return std::exp(x);
}

static double MathFloor(double x)
{
	return std::floor(x);
}

static double MathLog(double x)
{
	return std::log(x);
}

static Value MathMax(const std::vector<Value>& args)
{
	bool first = true;
	Value result = -INFINITY;

	BOOST_FOREACH(const Value& arg, args) {
		if (first || arg > result) {
			first = false;
			result = arg;
		}
	}

	return result;
}

static Value MathMin(const std::vector<Value>& args)
{
	bool first = true;
	Value result = INFINITY;

	BOOST_FOREACH(const Value& arg, args) {
		if (first || arg < result) {
			first = false;
			result = arg;
		}
	}

	return result;
}

static double MathPow(double x, double y)
{
	return std::pow(x, y);
}

static double MathRandom(void)
{
	return (double)std::rand() / RAND_MAX;
}

static double MathRound(double x)
{
	return boost::math::round(x);
}

static double MathSin(double x)
{
	return std::sin(x);
}

static double MathSqrt(double x)
{
	return std::sqrt(x);
}

static double MathTan(double x)
{
	return std::tan(x);
}

static bool MathIsnan(double x)
{
	return boost::math::isnan(x);
}

static bool MathIsinf(double x)
{
	return boost::math::isinf(x);
}

static double MathSign(double x)
{
	if (x > 0)
		return 1;
	else if (x < 0)
		return -1;
	else
		return 0;
}

static void InitializeMathObj(void)
{
	Dictionary::Ptr mathObj = new Dictionary();

	/* Constants */
	mathObj->Set("E", 2.71828182845904523536);
	mathObj->Set("LN2", 0.693147180559945309417);
	mathObj->Set("LN10", 2.30258509299404568402);
	mathObj->Set("LOG2E", 1.44269504088896340736);
	mathObj->Set("LOG10E", 0.434294481903251827651);
	mathObj->Set("PI", 3.14159265358979323846);
	mathObj->Set("SQRT1_2", 0.707106781186547524401);
	mathObj->Set("SQRT2", 1.41421356237309504880);

	/* Methods */
	mathObj->Set("abs", new Function(WrapFunction(MathAbs)));
	mathObj->Set("acos", new Function(WrapFunction(MathAcos)));
	mathObj->Set("asin", new Function(WrapFunction(MathAsin)));
	mathObj->Set("atan", new Function(WrapFunction(MathAtan)));
	mathObj->Set("atan2", new Function(WrapFunction(MathAtan2)));
	mathObj->Set("ceil", new Function(WrapFunction(MathCeil)));
	mathObj->Set("cos", new Function(WrapFunction(MathCos)));
	mathObj->Set("exp", new Function(WrapFunction(MathExp)));
	mathObj->Set("floor", new Function(WrapFunction(MathFloor)));
	mathObj->Set("log", new Function(WrapFunction(MathLog)));
	mathObj->Set("max", new Function(WrapFunction(MathMax)));
	mathObj->Set("min", new Function(WrapFunction(MathMin)));
	mathObj->Set("pow", new Function(WrapFunction(MathPow)));
	mathObj->Set("random", new Function(WrapFunction(MathRandom)));
	mathObj->Set("round", new Function(WrapFunction(MathRound)));
	mathObj->Set("sin", new Function(WrapFunction(MathSin)));
	mathObj->Set("sqrt", new Function(WrapFunction(MathSqrt)));
	mathObj->Set("tan", new Function(WrapFunction(MathTan)));
	mathObj->Set("isnan", new Function(WrapFunction(MathIsnan)));
	mathObj->Set("isinf", new Function(WrapFunction(MathIsinf)));
	mathObj->Set("sign", new Function(WrapFunction(MathSign)));

	ScriptGlobal::Set("Math", mathObj);
}

INITIALIZE_ONCE(InitializeMathObj);

