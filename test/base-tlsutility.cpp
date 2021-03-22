/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "base/tlsutility.hpp"
#include <BoostTestTargetConfig.h>
#include <utility>
#include <vector>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_tlsutility)

BOOST_AUTO_TEST_CASE(sha1)
{
	std::string allchars;
	for (size_t i = 0; i < 256; i++) {
		allchars.push_back(i);
	}

	std::vector<std::pair<std::string,std::string>> testdata = {
		{"",                        "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
		{"icinga",                  "f172c5e9e4d840a55356882a2b644846b302b216"},
		{"Icinga",                  "b3bdae77f60d9065f6152c7e3bbd351fa65e6fab"},
		{"ICINGA",                  "335da1d814abeef09b4623e2ce5169140c267a39"},
		{"#rX|wlcM:.8)uVmxz",       "99dc4d34caf36c6d6b08404135f1a7286211be1e"},
		{"AgbM;Z8Tz1!Im,kecZWs",    "aa793bef1ca307012980ae5ae046b7e929f6ed99"},
		{"yLUA4vKQ~24W}ahI;i?NLLS", "5e1a5ee3bd9fae5150681ef656ad43d9cb8e7005"},
		{allchars,                  "4916d6bdb7f78e6803698cab32d1586ea457dfc8"},
	};

	for (const auto& p : testdata) {
		const auto& input = p.first;
		const auto& expected = p.second;
		auto output = SHA1(input);
		BOOST_CHECK_MESSAGE(output == expected, "SHA1('" << input << "') should be " << expected << ", got " << output);
	}
}

BOOST_AUTO_TEST_SUITE_END()
