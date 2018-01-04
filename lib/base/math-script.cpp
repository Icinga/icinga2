/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

static double MathAtan2(double y, double x)
{
	return std::atan2(y, x);
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

	for (const Value& arg : args) {
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

	for (const Value& arg : args) {
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

static double MathRandom()
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

INITIALIZE_ONCE([]() {
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
	mathObj->Set("abs", new Function("Math#abs", MathAbs, { "x" }, true));
	mathObj->Set("acos", new Function("Math#acos", MathAcos, { "x" }, true));
	mathObj->Set("asin", new Function("Math#asin", MathAsin, { "x" }, true));
	mathObj->Set("atan", new Function("Math#atan", MathAtan, { "x" }, true));
	mathObj->Set("atan2", new Function("Math#atan2", MathAtan2, { "x", "y" }, true));
	mathObj->Set("ceil", new Function("Math#ceil", MathCeil, { "x" }, true));
	mathObj->Set("cos", new Function("Math#cos", MathCos, { "x" }, true));
	mathObj->Set("exp", new Function("Math#exp", MathExp, { "x" }, true));
	mathObj->Set("floor", new Function("Math#floor", MathFloor, { "x" }, true));
	mathObj->Set("log", new Function("Math#log", MathLog, { "x" }, true));
	mathObj->Set("max", new Function("Math#max", MathMax, {}, true));
	mathObj->Set("min", new Function("Math#min", MathMin, {}, true));
	mathObj->Set("pow", new Function("Math#pow", MathPow, { "x", "y" }, true));
	mathObj->Set("random", new Function("Math#random", MathRandom, {}, true));
	mathObj->Set("round", new Function("Math#round", MathRound, { "x" }, true));
	mathObj->Set("sin", new Function("Math#sin", MathSin, { "x" }, true));
	mathObj->Set("sqrt", new Function("Math#sqrt", MathSqrt, { "x" }, true));
	mathObj->Set("tan", new Function("Math#tan", MathTan, { "x" }, true));
	mathObj->Set("isnan", new Function("Math#isnan", MathIsnan, { "x" }, true));
	mathObj->Set("isinf", new Function("Math#isinf", MathIsinf, { "x" }, true));
	mathObj->Set("sign", new Function("Math#sign", MathSign, { "x" }, true));

	ScriptGlobal::Set("Math", mathObj);
});
