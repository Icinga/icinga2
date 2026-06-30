// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/macroprocessor.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_macros)

BOOST_AUTO_TEST_CASE(simple)
{
	Dictionary::Ptr macrosA = new Dictionary();
	macrosA->Set("testA", 7);
	macrosA->Set("testB", "hello");

	Dictionary::Ptr macrosB = new Dictionary();
	macrosB->Set("testA", 3);
	macrosB->Set("testC", "world");

	Array::Ptr testD = new Array();
	testD->Add(3);
	testD->Add("test");

	macrosB->Set("testD", testD);

	MacroProcessor::ResolverList resolvers;
	resolvers.emplace_back("macrosA", macrosA);
	resolvers.emplace_back("macrosB", macrosB);

	BOOST_CHECK(MacroProcessor::ResolveMacros("$macrosA.testB$ $macrosB.testC$", resolvers) == "hello world");
	BOOST_CHECK(MacroProcessor::ResolveMacros("$testA$", resolvers) == "7");
	BOOST_CHECK(MacroProcessor::ResolveMacros("$testA$$testB$", resolvers) == "7hello");

	Array::Ptr result = MacroProcessor::ResolveMacros("$testD$", resolvers);
	BOOST_CHECK(result->GetLength() == 2);

	/* verify the config validator macro checks */
	BOOST_CHECK(MacroProcessor::ValidateMacroString("$host.address") == false);
	BOOST_CHECK(MacroProcessor::ValidateMacroString("host.vars.test$") == false);

	BOOST_CHECK(MacroProcessor::ValidateMacroString("host.vars.test$") == false);
	BOOST_CHECK(MacroProcessor::ValidateMacroString("$template::test$abc$") == false);

	BOOST_CHECK(MacroProcessor::ValidateMacroString("$$test $host.vars.test$") == true);

	BOOST_CHECK(MacroProcessor::ValidateMacroString("test $host.vars.test$") == true);

}

BOOST_AUTO_TEST_SUITE_END()
