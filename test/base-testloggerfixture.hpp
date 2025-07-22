/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef TEST_LOGGER_FIXTURE_H
#define TEST_LOGGER_FIXTURE_H

#include "base/logger.hpp"
#include <boost/test/test_tools.hpp>
#include <boost/regex.hpp>
#include <future>
#include <BoostTestTargetConfig.h>

namespace icinga {

class TestLogger : public Logger
{
public:
	DECLARE_PTR_TYPEDEFS(TestLogger);

	struct Expect
	{
		std::string pattern;
		std::promise<bool> prom;
		std::shared_future<bool> crutch;
	};

	auto ExpectLogPattern(const std::string& pattern, const std::chrono::milliseconds& timeout = std::chrono::seconds(0))
	{
		std::unique_lock lock(m_Mutex);
		for (const auto & logEntry : m_LogEntries) {
			if (boost::regex_match(logEntry.Message.GetData(), boost::regex(pattern))) {
				return boost::test_tools::assertion_result{true};
			}
		}

		if (timeout == std::chrono::seconds(0)) {
			return boost::test_tools::assertion_result{false};
		}

		auto prom = std::promise<bool>();
		auto fut = prom.get_future().share();
		m_Expects.emplace_back(Expect{pattern, std::move(prom), fut});
		lock.unlock();

		auto status = fut.wait_for(timeout);
		boost::test_tools::assertion_result ret{status == std::future_status::ready && fut.get()};
		ret.message() << "Pattern \"" << pattern << "\" in log within " << timeout.count() << "ms";
		return ret;
	}

private:
	void ProcessLogEntry(const LogEntry& entry) override
	{
		std::unique_lock lock(m_Mutex);
		m_LogEntries.push_back(entry);

		m_Expects.erase(std::remove_if(m_Expects.begin(), m_Expects.end(), [&entry](Expect& expect) {
			if (boost::regex_match(entry.Message.GetData(), boost::regex(expect.pattern))) {
				expect.prom.set_value(true);
				return true;
			}
			return false;
		}), m_Expects.end());
	}

	void Flush() override {}

	std::mutex m_Mutex;
	std::vector<Expect> m_Expects;
	std::vector<LogEntry> m_LogEntries;
};

struct TestLoggerFixture
{
	TestLoggerFixture()
	{
		testLogger->SetSeverity(testLogger->SeverityToString(LogDebug));
		testLogger->Activate(true);
		testLogger->SetActive(true);
	}

	auto ExpectLogPattern(const std::string& pattern, const std::chrono::milliseconds& timeout = std::chrono::seconds(0))
	{
		return testLogger->ExpectLogPattern(pattern, timeout);
	}

	TestLogger::Ptr testLogger = new TestLogger;
};

} // namespace icinga

#endif // TEST_LOGGER_FIXTURE_H
