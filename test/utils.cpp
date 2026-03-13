// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "utils.hpp"
#include "base/perfdatavalue.hpp"
#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>
#include <boost/test/unit_test.hpp>

tm make_tm(std::string s)
{
    int dst = -1;
    size_t l = strlen("YYYY-MM-DD HH:MM:SS");
    if (s.size() > l) {
        std::string zone = s.substr(l);
        if (zone == " PST") {
            dst = 0;
        } else if (zone == " PDT") {
            dst = 1;
        } else {
            // tests should only use PST/PDT (for now)
            BOOST_CHECK_MESSAGE(false, "invalid or unknown time time: " << zone);
        }
    }

    std::tm t = {};
    std::istringstream stream(s);
    stream >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
    t.tm_isdst = dst;

    return t;
}

#ifndef _WIN32
const char *GlobalTimezoneFixture::TestTimezoneWithDST = "America/Los_Angeles";
#else /* _WIN32 */
// Tests are using pacific time because Windows only really supports timezones following US DST rules with the TZ
// environment variable. Format is "[Standard TZ][negative UTC offset][DST TZ]".
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/tzset?view=msvc-160#remarks
const char *GlobalTimezoneFixture::TestTimezoneWithDST = "PST8PDT";
#endif /* _WIN32 */

GlobalTimezoneFixture::GlobalTimezoneFixture(const char *fixed_tz)
{
    tz = getenv("TZ");
#ifdef _WIN32
    _putenv_s("TZ", fixed_tz == "" ? "UTC" : fixed_tz);
#else
    setenv("TZ", fixed_tz, 1);
#endif
    tzset();
}

GlobalTimezoneFixture::~GlobalTimezoneFixture()
{
#ifdef _WIN32
    if (tz)
        _putenv_s("TZ", tz);
    else
        _putenv_s("TZ", "");
#else
    if (tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
#endif
    tzset();
}

std::string GetRandomString(std::string prefix, std::size_t length)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> distribution('!', '~');

	for (auto i = 0U; i < length; i++) {
		prefix += static_cast<char>(distribution(gen));
	}

	return prefix;
}

/**
 * Make our test host receive a number of check-results.
 *
 * @param num The number of check-results to receive
 * @param state The state the check results should have
 * @param fn A function that will be passed the current check-result
 */
void ReceiveCheckResults(
	const icinga::Checkable::Ptr& host,
	std::size_t num,
	icinga::ServiceState state,
	const std::function<void(const icinga::CheckResult::Ptr&)>& fn
)
{
	using namespace icinga;

	StoppableWaitGroup::Ptr wg = new StoppableWaitGroup();

	for (auto i = 0UL; i < num; ++i) {
		CheckResult::Ptr cr = new CheckResult();

		cr->SetState(state);

		double now = Utility::GetTime();
		cr->SetActive(false);
		cr->SetScheduleStart(now);
		cr->SetScheduleEnd(now);
		cr->SetExecutionStart(now);
		cr->SetExecutionEnd(now);

		Array::Ptr perfData = new Array;
		perfData->Add(new PerfdataValue{"dummy", 42});
		cr->SetPerformanceData(perfData);

		if (fn) {
			fn(cr);
		}

		BOOST_REQUIRE(host->ProcessCheckResult(cr, wg) == Checkable::ProcessingResult::Ok);
	}
}
