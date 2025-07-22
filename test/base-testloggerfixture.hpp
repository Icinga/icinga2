/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef TEST_LOGGER_FIXTURE_H
#define TEST_LOGGER_FIXTURE_H

#include <BoostTestTargetConfig.h>
#include "base/logger.hpp"
#include <boost/range/algorithm.hpp>
#include <boost/regex.hpp>
#include <boost/test/test_tools.hpp>
#include <future>

namespace icinga {

class TestLogger : public Logger
{
public:
	DECLARE_PTR_TYPEDEFS(TestLogger);

	struct Expect
	{
		std::string pattern;
		std::promise<bool> prom;
	};

	auto ExpectLogPattern(const std::string& pattern,
		const std::chrono::milliseconds& timeout = std::chrono::seconds(0))
	{
		std::unique_lock lock(m_Mutex);
		for (const auto& logEntry : m_LogEntries) {
			if (boost::regex_match(logEntry.Message.GetData(), boost::regex(pattern))) {
				return boost::test_tools::assertion_result{true};
			}
		}

		if (timeout == std::chrono::seconds(0)) {
			return boost::test_tools::assertion_result{false};
		}

		auto expect = std::make_shared<Expect>(Expect{pattern, std::promise<bool>()});
		m_Expects.emplace_back(expect);
		lock.unlock();

		auto future = expect->prom.get_future();
		auto status = future.wait_for(timeout);
		boost::test_tools::assertion_result ret{status == std::future_status::ready && future.get()};
		ret.message() << "Pattern \"" << pattern << "\" in log within " << timeout.count() << "ms";

		lock.lock();
		m_Expects.erase(boost::range::remove(m_Expects, expect), m_Expects.end());

		return ret;
	}

private:
	void ProcessLogEntry(const LogEntry& entry) override
	{
		std::unique_lock lock(m_Mutex);
		m_LogEntries.push_back(entry);

		auto it = boost::range::remove_if(m_Expects, [&entry](const std::shared_ptr<Expect>& expect) {
			if (boost::regex_match(entry.Message.GetData(), boost::regex(expect->pattern))) {
				expect->prom.set_value(true);
				return true;
			}
			return false;
		});
		m_Expects.erase(it, m_Expects.end());
	}

	void Flush() override {}

	std::mutex m_Mutex;
	std::vector<std::shared_ptr<Expect>> m_Expects;
	std::vector<LogEntry> m_LogEntries;
};

/**
 * A fixture to capture log entries and assert their presence in tests.
 *
 * Currently, this only supports checking existing entries and waiting for new ones
 * using ExpectLogPattern(), but more functionality can easily be added in the future,
 * like only asserting on past log messages, only waiting for new ones, asserting log
 * entry metadata (severity etc.) and so on.
 */
struct TestLoggerFixture
{
	TestLoggerFixture()
	{
		testLogger->SetSeverity(testLogger->SeverityToString(LogDebug));
		testLogger->Activate(true);
		testLogger->SetActive(true);
	}

	~TestLoggerFixture()
	{
		testLogger->SetActive(false);
		testLogger->Deactivate(true);
	}

	/**
	 * Asserts the presence of a log entry that matches the given regex pattern.
	 *
	 * First, the existing log entries are searched for the pattern. If the pattern isn't found,
	 * until the timeout is reached, the function will wait if a new log message is added that
	 * matches the pattern.
	 *
	 * A boost assertion result object is returned, that evaluates to bool, but contains an
	 * error message that is printed by the testing framework when the assert failed.
	 *
	 * @param pattern The regex pattern the log message needs to match
	 * @param timeout The maximum amount of time to wait for the log message to arrive
	 *
	 * @return A @c boost::test_tools::assertion_result object that can be used in BOOST_REQUIRE
	 */
	auto ExpectLogPattern(const std::string& pattern,
		const std::chrono::milliseconds& timeout = std::chrono::seconds(0))
	{
		return testLogger->ExpectLogPattern(pattern, timeout);
	}

	TestLogger::Ptr testLogger = new TestLogger;
};

} // namespace icinga

#endif // TEST_LOGGER_FIXTURE_H
