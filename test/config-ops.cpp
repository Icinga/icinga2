/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "config/configcompiler.hpp"
#include "base/exception.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(config_ops)

BOOST_AUTO_TEST_CASE(simple)
{
	ScriptFrame frame(true);
	std::unique_ptr<Expression> expr;
	Dictionary::Ptr dict;

	expr = ConfigCompiler::CompileText("<test>", "");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == Empty);

	expr = ConfigCompiler::CompileText("<test>", "\n3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "{ 3\n\n5 }");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	expr = ConfigCompiler::CompileText("<test>", "1 + 3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 4);

	expr = ConfigCompiler::CompileText("<test>", "3 - 1");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 2);

	expr = ConfigCompiler::CompileText("<test>", "5m * 10");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3000);

	expr = ConfigCompiler::CompileText("<test>", "5m / 5");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 60);

	expr = ConfigCompiler::CompileText("<test>", "7 & 3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "2 | 3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "true && false");
	BOOST_CHECK(!expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "true || false");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "3 < 5");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "3 > 5");
	BOOST_CHECK(!expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "3 <= 3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "3 >= 3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "2 + 3 * 4");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 14);

	expr = ConfigCompiler::CompileText("<test>", "(2 + 3) * 4");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 20);

	expr = ConfigCompiler::CompileText("<test>", "2 * - 3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == -6);

	expr = ConfigCompiler::CompileText("<test>", "-(2 + 3)");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == -5);

	expr = ConfigCompiler::CompileText("<test>", "- 2 * 2 - 2 * 3 - 4 * - 5");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 10);

	expr = ConfigCompiler::CompileText("<test>", "!0 == true");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "~0");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == (double)~(long)0);

	expr = ConfigCompiler::CompileText("<test>", "4 << 8");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 1024);

	expr = ConfigCompiler::CompileText("<test>", "1024 >> 4");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 64);

	expr = ConfigCompiler::CompileText("<test>", "2 << 3 << 4");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 256);

	expr = ConfigCompiler::CompileText("<test>", "256 >> 4 >> 3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 2);

	expr = ConfigCompiler::CompileText("<test>", R"("hello" == "hello")");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", R"("hello" != "hello")");
	BOOST_CHECK(!expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", R"("foo" in [ "foo", "bar" ])");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", R"("foo" in [ "bar", "baz" ])");
	BOOST_CHECK(!expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "\"foo\" in null");
	BOOST_CHECK(!expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", R"("foo" in "bar")");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	expr = ConfigCompiler::CompileText("<test>", R"("foo" !in [ "bar", "baz" ])");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", R"("foo" !in [ "foo", "bar" ])");
	BOOST_CHECK(!expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "\"foo\" !in null");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", R"("foo" !in "bar")");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	expr = ConfigCompiler::CompileText("<test>", "{ a += 3 }");
	dict = expr->Evaluate(frame).GetValue();
	BOOST_CHECK(dict->GetLength() == 1);
	BOOST_CHECK(dict->Get("a") == 3);

	expr = ConfigCompiler::CompileText("<test>", "test");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	expr = ConfigCompiler::CompileText("<test>", "null + 3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "3 + null");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "\"test\" + 3");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == "test3");

	expr = ConfigCompiler::CompileText("<test>", R"("\"te\\st")");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == "\"te\\st");

	expr = ConfigCompiler::CompileText("<test>", R"("\'test")");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	expr = ConfigCompiler::CompileText("<test>", "({ a = 3\nb = 3 })");
	BOOST_CHECK(expr->Evaluate(frame).GetValue().IsObjectType<Dictionary>());
}

BOOST_AUTO_TEST_CASE(advanced)
{
	ScriptFrame frame(true);
	std::unique_ptr<Expression> expr;
	Function::Ptr func;

	expr = ConfigCompiler::CompileText("<test>", R"(regex("^Hello", "Hello World"))");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "__boost_test()");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	Object::Ptr self = new Object();
	ScriptFrame frame2(true, self);
	expr = ConfigCompiler::CompileText("<test>", "this");
	BOOST_CHECK(expr->Evaluate(frame2).GetValue() == Value(self));

	expr = ConfigCompiler::CompileText("<test>", "var v = 7; v");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "{ a = 3 }.a");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "[ 2, 3 ][1]");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "var v = { a = 3}; v.a");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "a = 3 b = 3");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	expr = ConfigCompiler::CompileText("<test>", "function() { 3 }()");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "function() { return 3, 5 }()");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "typeof([]) == Array");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "typeof({}) == Dictionary");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "typeof(3) == Number");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "typeof(\"test\") == String");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "(7 | 8) == 15");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "(7 ^ 8) == 15");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "(7 & 15) == 7");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "7 in [7] == true");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "7 !in [7] == false");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "(7 | 8) > 14");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "(7 ^ 8) > 14");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "(7 & 15) > 6");
	BOOST_CHECK(expr->Evaluate(frame).GetValue());

	expr = ConfigCompiler::CompileText("<test>", "\"a\" = 3");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	expr = ConfigCompiler::CompileText("<test>", "3 = 3");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	expr = ConfigCompiler::CompileText("<test>", "var e; e");
	BOOST_CHECK(expr->Evaluate(frame).GetValue().IsEmpty());

	expr = ConfigCompiler::CompileText("<test>", "var e = 3; e");
	BOOST_CHECK(expr->Evaluate(frame).GetValue() == 3);

	expr = ConfigCompiler::CompileText("<test>", "Array.x");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);

	expr = ConfigCompiler::CompileText("<test>", "{{ 3 }}");
	func = expr->Evaluate(frame).GetValue();
	BOOST_CHECK(func->Invoke() == 3);

	// Regression test for CVE-2025-61908
	expr = ConfigCompiler::CompileText("<test>", "&*null");
	BOOST_CHECK_THROW(expr->Evaluate(frame).GetValue(), ScriptError);
}

BOOST_AUTO_TEST_SUITE_END()
