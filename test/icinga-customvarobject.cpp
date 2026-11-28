// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <BoostTestTargetConfig.h>
#include "base/dictionary.hpp"
#include "base/exception.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/host.hpp"

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_customvarobject)

BOOST_AUTO_TEST_CASE(modify_attribute_depth_limit)
{
	auto NewObj = []() -> CustomVarObject::Ptr { return new CustomVarObject; };

	// Create dictionaries of different nesting depths for the tests. nested[i] contains i layers of dictionaries.
	std::array<Value, 17> nested = {Empty};
	for (std::size_t i = 1; i < nested.size(); i++) {
		nested[i] = new Dictionary{{std::to_string(i), nested[i - 1]}};
	}

	// Test nesting created using only d.o.t.t.e.d syntax.
	BOOST_CHECK_NO_THROW(NewObj()->ModifyAttribute("vars.1.2.3.4.5.6.7.8.9.10.11.12.13.14.15", Empty));
	BOOST_CHECK_EXCEPTION(
		NewObj()->ModifyAttribute("vars.1.2.3.4.5.6.7.8.9.10.11.12.13.14.15.16", Empty),
		std::invalid_argument,
		[](auto ex) {
			boost::test_tools::assertion_result result{
				std::string_view(ex.what()).find("exceeds maximum nesting level of 16") != std::string_view::npos
			};
			result.message() << "got exception: " << ex.what();
			return result;
		}
	);

	// Test nesting created using only nested dictionaries.
	BOOST_CHECK_NO_THROW(NewObj()->ModifyAttribute("vars", nested[15]));
	BOOST_CHECK_EXCEPTION(NewObj()->ModifyAttribute("vars", nested[16]), ValidationError, [](auto ex) {
		boost::test_tools::assertion_result result{
			std::string_view(ex.what()).find("Variables nested too deep.") != std::string_view::npos
		};
		result.message() << "got exception: " << ex.what();
		return result;
	});

	// Test nesting created by a mix of d.o.t.t.e.d syntax and mixed dictionaries.
	BOOST_CHECK_NO_THROW(NewObj()->ModifyAttribute("vars.1.2.3.4.5", nested[10]));
	BOOST_CHECK_EXCEPTION(NewObj()->ModifyAttribute("vars.1.2.3.4.5", nested[11]), ValidationError, [](auto ex) {
		boost::test_tools::assertion_result result{
			std::string_view(ex.what()).find("Variables nested too deep.") != std::string_view::npos
		};
		result.message() << "got exception: " << ex.what();
		return result;
	});
	BOOST_CHECK_EXCEPTION(NewObj()->ModifyAttribute("vars.1.2.3.4.5.6", nested[10]), ValidationError, [](auto ex) {
		boost::test_tools::assertion_result result{
			std::string_view(ex.what()).find("Variables nested too deep.") != std::string_view::npos
		};
		result.message() << "got exception: " << ex.what();
		return result;
	});
}

BOOST_AUTO_TEST_SUITE_END()
