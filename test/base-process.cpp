/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#include "base/process.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_process)

BOOST_AUTO_TEST_CASE(e2big)
{
#ifndef _WIN32
	String placeHolder = "x";

	for (int i = 0; i < 23; ++i) {
		placeHolder += placeHolder; // 2^23 = 8388608 > ARG_MAX

		Process::Ptr p = new Process(
			{"echo", "truncated_" + placeHolder + "Y", "untouched_despite_longer_" + placeHolder + "Z"},
			nullptr,
			new Array({"d_" + placeHolder + "Y"})
		);

		p->SetTimeout(60 * 60);
		p->Run();

		auto& out (p->WaitForResult().Output);

		BOOST_CHECK(out.Contains("truncate"));
		BOOST_CHECK(out.Contains(" untouched_despite_longer_x"));
		BOOST_CHECK(out.Contains("xZ"));

		if (!out.Contains("Y")) {
			return;
		}
	}

	BOOST_WARN_MESSAGE(false, "exec(3) allowed 16MB+ argv, so truncation wasn't necessary nor tested!");
#endif /* _WIN32 */
}

BOOST_AUTO_TEST_SUITE_END()
