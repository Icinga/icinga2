/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/initialize.hpp"
#include "base/namespace.hpp"
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
	Namespace::Ptr mathNS = new Namespace(true);

	/* Constants */
	mathNS->Set("E", 2.71828182845904523536);
	mathNS->Set("LN2", 0.693147180559945309417);
	mathNS->Set("LN10", 2.30258509299404568402);
	mathNS->Set("LOG2E", 1.44269504088896340736);
	mathNS->Set("LOG10E", 0.434294481903251827651);
	mathNS->Set("PI", 3.14159265358979323846);
	mathNS->Set("SQRT1_2", 0.707106781186547524401);
	mathNS->Set("SQRT2", 1.41421356237309504880);

	/* Methods */
	mathNS->Set("abs", new Function("Math#abs", MathAbs, { "x" }, true));
	mathNS->Set("acos", new Function("Math#acos", MathAcos, { "x" }, true));
	mathNS->Set("asin", new Function("Math#asin", MathAsin, { "x" }, true));
	mathNS->Set("atan", new Function("Math#atan", MathAtan, { "x" }, true));
	mathNS->Set("atan2", new Function("Math#atan2", MathAtan2, { "x", "y" }, true));
	mathNS->Set("ceil", new Function("Math#ceil", MathCeil, { "x" }, true));
	mathNS->Set("cos", new Function("Math#cos", MathCos, { "x" }, true));
	mathNS->Set("exp", new Function("Math#exp", MathExp, { "x" }, true));
	mathNS->Set("floor", new Function("Math#floor", MathFloor, { "x" }, true));
	mathNS->Set("log", new Function("Math#log", MathLog, { "x" }, true));
	mathNS->Set("max", new Function("Math#max", MathMax, {}, true));
	mathNS->Set("min", new Function("Math#min", MathMin, {}, true));
	mathNS->Set("pow", new Function("Math#pow", MathPow, { "x", "y" }, true));
	mathNS->Set("random", new Function("Math#random", MathRandom, {}, true));
	mathNS->Set("round", new Function("Math#round", MathRound, { "x" }, true));
	mathNS->Set("sin", new Function("Math#sin", MathSin, { "x" }, true));
	mathNS->Set("sqrt", new Function("Math#sqrt", MathSqrt, { "x" }, true));
	mathNS->Set("tan", new Function("Math#tan", MathTan, { "x" }, true));
	mathNS->Set("isnan", new Function("Math#isnan", MathIsnan, { "x" }, true));
	mathNS->Set("isinf", new Function("Math#isinf", MathIsinf, { "x" }, true));
	mathNS->Set("sign", new Function("Math#sign", MathSign, { "x" }, true));

	mathNS->Freeze();

	Namespace::Ptr systemNS = ScriptGlobal::Get("System");
	systemNS->SetAttribute("Math", new ConstEmbeddedNamespaceValue(mathNS));
});
