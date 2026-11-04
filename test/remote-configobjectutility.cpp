// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <BoostTestTargetConfig.h>
#include "base/dictionary.hpp"
#include "icinga/host.hpp"
#include "remote/configobjectutility.hpp"

using namespace icinga;

BOOST_AUTO_TEST_SUITE(remote_configobjectutility)

BOOST_AUTO_TEST_CASE(create_object_depth_limit)
{
	BOOST_CHECK_NO_THROW(
		ConfigObjectUtility::CreateObjectConfig(
			Host::TypeInstance,
			"test",
			false,
			nullptr,
			new Dictionary{
				{"vars.1.2.3.4.5.6.7.8.9.10.11.12.13.14.15", 42},
			}
		)
	);
	BOOST_CHECK_EXCEPTION(
		ConfigObjectUtility::CreateObjectConfig(
			Host::TypeInstance,
			"test",
			false,
			nullptr,
			new Dictionary{
				{"vars.1.2.3.4.5.6.7.8.9.10.11.12.13.14.15.16", 42},
			}
		),
		std::invalid_argument,
		[](auto ex) {
			boost::test_tools::assertion_result result{
				std::string_view(ex.what()).find("exceeds maximum nesting level of 16") != std::string_view::npos
			};
			result.message() << "got exception: " << ex.what();
			return result;
		}
	);
}

BOOST_AUTO_TEST_SUITE_END()
