// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <BoostTestTargetConfig.h>
#include "base/perfdatavalue.hpp"
#include "config/configcompiler.hpp"
#include "config/configitem.hpp"
#include "icinga/host.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatatargetfixture.hpp"
#include "test/utils.hpp"
#include <boost/hana.hpp>

namespace icinga {

template<typename Writer>
class PerfdataWriterFixture : public PerfdataWriterTargetFixture, public TestLoggerFixture
{
public:
	PerfdataWriterFixture() : m_Writer(new Writer)
	{
		auto createObjects = [&]() {
			String config = R"CONFIG(
object CheckCommand "dummy" {
	command = "/bin/echo"
}
object Host "h1" {
	address = "h1"
	check_command = "dummy"
	enable_notifications = true
	enable_active_checks = false
	enable_passive_checks = true
}
)CONFIG";

			std::unique_ptr<Expression> expr = ConfigCompiler::CompileText("<test>", config);
			expr->Evaluate(*ScriptFrame::GetCurrentFrame());
		};

		ConfigItem::RunWithActivationContext(new Function("CreateTestObjects", createObjects));

		m_Host = Host::GetByName("h1");
		BOOST_REQUIRE(m_Host);

		m_Writer->SetPort(std::to_string(GetPort()));
		m_Writer->SetName(m_Writer->GetReflectionType()->GetName());
		m_Writer->SetDisconnectTimeout(0.05);
		m_Writer->Register();

		auto hasFlushInterval = boost::hana::is_valid([](auto&& obj) -> decltype(obj.SetFlushInterval(0.05)) {});
		if constexpr (decltype(hasFlushInterval(std::declval<Writer>()))::value) {
			m_Writer->SetFlushInterval(0.05);
		}

		auto hasFlushThreshold = boost::hana::is_valid([](auto&& obj) -> decltype(obj.SetFlushThreshold(1)) {});
		if constexpr (decltype(hasFlushThreshold(std::declval<Writer>()))::value) {
			m_Writer->SetFlushThreshold(1);
		}
	}

	void ReceiveCheckResults(
		std::size_t num,
		ServiceState state,
		const std::function<void(const CheckResult::Ptr&)>& fn = {}
	)
	{
		::ReceiveCheckResults(m_Host, num, state, fn);
	}

	std::size_t GetWorkQueueLength()
	{
		Array::Ptr dummy = new Array;
		Dictionary::Ptr status = new Dictionary;
		m_Writer->StatsFunc(status, dummy);
		ObjectLock lock{status};
		// Unpack the single-key top-level dictionary
		Dictionary::Ptr writer = status->Begin()->second;
		BOOST_REQUIRE(writer);
		Dictionary::Ptr values = writer->Get(m_Writer->GetName());
		BOOST_REQUIRE(values);
		BOOST_REQUIRE(values->Contains("work_queue_items"));
		return values->Get("work_queue_items");
	}

	/**
	 * Processes check results until the writer's work queue is no longer moving.
	 *
	 * @param timeout Time after which to give up trying to get the writer stuck
	 * @return true if the writer is now stuck
	 */
	bool GetWriterStuck(std::chrono::milliseconds timeout)
	{
		auto start = std::chrono::steady_clock::now();
		std::size_t unchangedCount = 0;
		while(true){
			ReceiveCheckResults(10, ServiceCritical, [&](const CheckResult::Ptr& cr) {
				cr->GetPerformanceData()->Add(new PerfdataValue{GetRandomString("", 4096), 1});
			});

			if (std::chrono::steady_clock::now() - start >= timeout) {
				return false;
			}

			auto numWq = GetWorkQueueLength();
			if (numWq >= 10) {
				std::this_thread::sleep_for(1ms);
				if (numWq == GetWorkQueueLength()) {
					if (unchangedCount < 5) {
						++unchangedCount;
						continue;
					}
					return true;
				}

				unchangedCount = 0;
			}
		}
	}

	void ResumeWriter()
	{
		static_cast<ConfigObject::Ptr>(m_Writer)->OnConfigLoaded();
		m_Writer->SetActive(true);
		m_Writer->Activate();
		BOOST_REQUIRE(!m_Writer->IsPaused());
	}

	void PauseWriter() { static_cast<ConfigObject::Ptr>(m_Writer)->Pause(); }

	auto GetWriter() { return m_Writer; }

private:
	Host::Ptr m_Host;
	typename Writer::Ptr m_Writer;
};

} // namespace icinga
