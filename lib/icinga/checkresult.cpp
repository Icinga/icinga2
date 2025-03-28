/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/checkresult.hpp"
#include "icinga/checkresult-ti.cpp"
#include "base/scriptglobal.hpp"

using namespace icinga;

REGISTER_TYPE(CheckResult);

INITIALIZE_ONCE([]() {
	ScriptGlobal::Set("Icinga.ServiceOK", ServiceOK);
	ScriptGlobal::Set("Icinga.ServiceWarning", ServiceWarning);
	ScriptGlobal::Set("Icinga.ServiceCritical", ServiceCritical);
	ScriptGlobal::Set("Icinga.ServiceUnknown", ServiceUnknown);

	ScriptGlobal::Set("Icinga.HostUp", HostUp);
	ScriptGlobal::Set("Icinga.HostDown", HostDown);
})

double CheckResult::CalculateExecutionTime() const
{
	return GetExecutionEnd() - GetExecutionStart();
}

double CheckResult::CalculateLatency() const
{
	double latency = (GetScheduleEnd() - GetScheduleStart()) - CalculateExecutionTime();

	if (latency < 0)
		latency = 0;

	return latency;
}

bool CheckResultProducerComponent::try_lock_shared() noexcept
{
	auto expected (m_State.load());
	decltype(expected) desired;

	do {
		if (!expected.m_InstanceIsActive) {
			return false;
		}

		desired = expected;
		++desired.m_ProcessingCheckResults;
	} while (!m_State.compare_exchange_weak(expected, desired));

	return true;
}

void CheckResultProducerComponent::unlock_shared() noexcept
{
	auto state (ModifyState([](auto& desired) { --desired.m_ProcessingCheckResults; }));

	if (!state.m_ProcessingCheckResults) {
		m_CV.notify_all();
	}
}

/**
 * Allow processing check results.
 */
void CheckResultProducerComponent::Start()
{
	ModifyState([](auto& desired) { desired.m_InstanceIsActive = 1; });
}

/**
 * Disallow processing new check results, wait for all currently processed ones to finish.
 */
void CheckResultProducerComponent::Stop()
{
	ModifyState([](auto& desired) { desired.m_InstanceIsActive = 0; });

	std::unique_lock lock (m_Mutex);
	m_CV.wait(lock, [this] { return !m_State.load().m_ProcessingCheckResults; });
}

ObjectFactory TypeHelper<CheckResult, false>::GetFactory()
{
	return &Factory;
}

Object::Ptr TypeHelper<CheckResult, false>::Factory(const std::vector<Value>&)
{
	return new CheckResult(DslCheckResultProducer::m_Instance);
}

bool DslCheckResultProducer::try_lock_shared() noexcept
{
	return false;
}

void DslCheckResultProducer::unlock_shared() noexcept
{
}

CheckResultProducer::Ptr DslCheckResultProducer::m_Instance = new DslCheckResultProducer();

bool TestCheckResultProducer::try_lock_shared() noexcept
{
	return true;
}

void TestCheckResultProducer::unlock_shared() noexcept
{
}
