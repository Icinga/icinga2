/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "remote/configpackageutility.hpp"
#include <vector>
#include <string>
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(remote_configpackageutility)

BOOST_AUTO_TEST_CASE(ValidateName)
{
	std::vector<std::string> validNames {"foo", "foo-bar", "FooBar", "Foo123", "_Foo-", "123bar"};
	for (const std::string& n : validNames) {
		BOOST_CHECK_MESSAGE(ConfigPackageUtility::ValidatePackageName(n), "'" << n << "' should be valid");
	}

	std::vector<std::string> invalidNames {"", ".", "..", "foo.bar", "foo/../bar", "foo/bar", "foo:bar"};
	for (const std::string& n : invalidNames) {
		BOOST_CHECK_MESSAGE(!ConfigPackageUtility::ValidatePackageName(n), "'" << n << "' should not be valid");
	}
}

BOOST_AUTO_TEST_SUITE_END()
