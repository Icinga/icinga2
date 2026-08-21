// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TEST_LOGGER_FIXTURE_H
#define TEST_LOGGER_FIXTURE_H

#include <BoostTestTargetConfig.h>
#include "base/logger.hpp"
#include <boost/range/algorithm.hpp>
#include <boost/regex.hpp>
#include <boost/test/test_tools.hpp>
#include <future>

#define CHECK_LOG_PATTERN(pattern, timeout) BOOST_CHECK(ExpectLogPattern(pattern, timeout))
#define REQUIRE_LOG_PATTERN(pattern, timeout) BOOST_REQUIRE(ExpectLogPattern(pattern, timeout))
#define CHECK_LOG_MESSAGE(pattern, timeout) BOOST_CHECK(ExpectLogMessage(pattern, timeout))
#define REQUIRE_LOG_MESSAGE(pattern, timeout) BOOST_REQUIRE(ExpectLogMessage(pattern, timeout))

#define CHECK_NO_LOG_PATTERN(pattern, timeout) BOOST_CHECK(!ExpectLogPattern(pattern, timeout))
#define REQUIRE_NO_LOG_PATTERN(pattern, timeout) BOOST_REQUIRE(!ExpectLogPattern(pattern, timeout))
#define CHECK_NO_LOG_MESSAGE(pattern, timeout) BOOST_CHECK(!ExpectLogMessage(pattern, timeout))
#define REQUIRE_NO_LOG_MESSAGE(pattern, timeout) BOOST_REQUIRE(!ExpectLogMessage(pattern, timeout))

namespace icinga {

class TestLogger : public Logger
{
public:
	DECLARE_PTR_TYPEDEFS(TestLogger);

	using MatchPredicate = std::function<bool(const LogEntry&)>;

	bool ExpectLogPredicate(MatchPredicate pred, std::chrono::milliseconds timeout)
	{
		std::unique_lock lock(m_Mutex);
		for (const auto& logEntry : m_LogEntries) {
			if (pred(logEntry)) {
				return true;
			}
		}

		if (timeout == 0s) {
			return false;
		}

		auto expect = std::make_shared<Expect>(Expect{std::move(pred), std::promise<bool>()});
		m_Expects.emplace_back(expect);
		lock.unlock();

		auto future = expect->prom.get_future();
		auto status = future.wait_for(timeout);

		lock.lock();
		m_Expects.erase(boost::range::remove(m_Expects, expect), m_Expects.end());

		return status == std::future_status::ready && future.get();
	}

	void Clear()
	{
		std::lock_guard lock(m_Mutex);
		m_Expects.clear();
		m_LogEntries.clear();
	}

	void ProcessLogEntry(const LogEntry& entry) override
	{
		std::unique_lock lock(m_Mutex);
		m_LogEntries.push_back(entry);

		auto it = boost::range::remove_if(m_Expects, [&entry](const std::shared_ptr<Expect>& expect) {
			if (expect->pred(entry)) {
				expect->prom.set_value(true);
				return true;
			}
			return false;
		});
		m_Expects.erase(it, m_Expects.end());
	}

	void Flush() override {}

private:
	struct Expect {
		MatchPredicate pred;
		std::promise<bool> prom;
	};

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
		testLogger->SetActive(true);
		testLogger->Activate(true);
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
	[[nodiscard]] auto ExpectLogPattern(const std::string& pattern, std::chrono::milliseconds timeout = 0s) const
	{
		bool found = testLogger->ExpectLogPredicate(
			[re = boost::regex(pattern)](const LogEntry& entry) {
				return boost::regex_match(entry.Message.GetData(), re);
			},
			timeout
		);

		boost::test_tools::assertion_result ret{found};
		ret.message() << "Pattern \"" << pattern << "\" in log within " << timeout.count() << "ms";
		return ret;
	}

	/**
	 * Asserts the presence of a log entry that matches the given string exactly.
	 *
	 * First, the existing log entries are searched for the message. If the message isn't found,
	 * until the timeout is reached, the function will wait if a new log message is added that
	 * matches the desired message.
	 *
	 * A boost assertion result object is returned, that evaluates to bool, but contains an
	 * error message that is printed by the testing framework when the assert failed.
	 *
	 * @param message The exact string the log message needs to match
	 * @param timeout The maximum amount of time to wait for the log message to arrive
	 *
	 * @return A @c boost::test_tools::assertion_result object that can be used in BOOST_REQUIRE
	 */
	[[nodiscard]] auto ExpectLogMessage(std::string message, std::chrono::milliseconds timeout = 0s) const
	{
		bool found = testLogger->ExpectLogPredicate(
			[pattern = std::move(message)](const LogEntry& entry) { return entry.Message.GetData() == pattern; }, timeout
		);

		boost::test_tools::assertion_result ret{found};
		ret.message() << "Pattern \"" << message << "\" in log within " << timeout.count() << "ms";
		return ret;
	}

	void ClearTestLogger() const { testLogger->Clear(); }

	TestLogger::Ptr testLogger = new TestLogger;
};

} // namespace icinga

#endif // TEST_LOGGER_FIXTURE_H
