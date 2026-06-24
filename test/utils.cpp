/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "utils.hpp"
#include "base/io-engine.hpp"
#include <cstring>
#include <future>
#include <iomanip>
#include <sstream>
#include <boost/asio/spawn.hpp>
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

std::future<void> SpawnSynchronizedCoroutine(std::function<void(boost::asio::yield_context)> fn)
{
	using namespace icinga;

	auto promise = std::make_unique<std::promise<void>>();
	auto future = promise->get_future();
	auto& io = IoEngine::Get().GetIoContext();
	IoEngine::SpawnCoroutine(io, [promise = std::move(promise), fn = std::move(fn)](boost::asio::yield_context yc) {
		try {
			fn(std::move(yc));
		} catch (const std::exception&) {
			promise->set_exception(std::current_exception());
			return;
		}
		promise->set_value();
	});
	return future;
}
