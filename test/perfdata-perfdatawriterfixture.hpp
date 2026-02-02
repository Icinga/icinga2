/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include <BoostTestTargetConfig.h>
#include "base/perfdatavalue.hpp"
#include "base/wait-group.hpp"
#include "config/configcompiler.hpp"
#include "config/configitem.hpp"
#include "icinga/host.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatatargetfixture.hpp"
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

		auto hasFlushInterval = boost::hana::is_valid([](auto&& obj) -> decltype(obj.SetFlushInterval(0.05)) {});
		if constexpr (decltype(hasFlushInterval(std::declval<Writer>()))::value) {
			m_Writer->SetFlushInterval(0.05);
		}

		auto hasFlushThreshold = boost::hana::is_valid([](auto&& obj) -> decltype(obj.SetFlushThreshold(1)) {});
		if constexpr (decltype(hasFlushThreshold(std::declval<Writer>()))::value) {
			m_Writer->SetFlushThreshold(1);
		}
	}

	/**
	 * Make our test host receive a number of check-results.
	 *
	 * @param num The number of check-results to receive
	 * @param state The state the check results should have
	 * @param fn A function that will be passed the current check-result
	 */
	void ReceiveCheckResults(std::size_t num, ServiceState state, const std::function<void(const CheckResult::Ptr&)>& fn = {})
	{
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
			perfData->Add(new PerfdataValue{"thing", 42});
			cr->SetPerformanceData(perfData);

			if (fn) {
				fn(cr);
			}

			BOOST_REQUIRE(m_Host->ProcessCheckResult(cr, wg) == Checkable::ProcessingResult::Ok);
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
