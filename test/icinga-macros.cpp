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
