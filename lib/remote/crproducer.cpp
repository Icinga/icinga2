/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "remote/crproducer.hpp"

using namespace icinga;

bool CheckResultProducerComponent::try_lock_shared() noexcept
{
	auto state (ModifyState(
		[](auto current) { return current.InstanceIsActive; },
		[](auto& desired) { ++desired.ProcessingCheckResults; }
	));

	return state.InstanceIsActive;
}

void CheckResultProducerComponent::unlock_shared() noexcept
{
	std::unique_lock lock (m_Mutex, std::defer_lock);

	auto state (ModifyState([&lock](auto& desired) {
		--desired.ProcessingCheckResults;

		if (!desired.ProcessingCheckResults && !desired.InstanceIsActive && !lock) {
			lock.lock();
		}
	}));

	if (!state.ProcessingCheckResults && !state.InstanceIsActive) {
		m_CV.notify_all();
	}
}

/**
 * Allow processing check results.
 */
void CheckResultProducerComponent::Start()
{
	ModifyState([](auto& desired) { desired.InstanceIsActive = 1; });
}

/**
 * Disallow processing new check results, wait for all currently processed ones to finish.
 */
void CheckResultProducerComponent::Stop()
{
	std::unique_lock lock (m_Mutex, std::defer_lock);

	auto state (ModifyState([&lock](auto& desired) {
		desired.InstanceIsActive = 0;

		if (desired.ProcessingCheckResults && !lock) {
			lock.lock();
		}
	}));

	if (state.ProcessingCheckResults) {
		m_CV.wait(lock, [this] { return !m_State.load().ProcessingCheckResults; });
	}
}

/**
 * Load m_State into x and if cond(x), pass x to mod by reference and try to store x back.
 * If m_State has changed in the meantime, repeat the process.
 *
 * @return The (not) updated m_State.
 */
template<class C, class M>
inline
CheckResultProducerComponent::State CheckResultProducerComponent::ModifyState(const C& cond, const M& mod)
{
	auto expected (m_State.load());
	decltype(expected) desired;

	do {
		if (!cond(expected)) {
			return expected;
		}

		desired = expected;
		mod(desired);
	} while (!m_State.compare_exchange_weak(expected, desired));

	return desired;
}

template<class M>
inline
CheckResultProducerComponent::State CheckResultProducerComponent::ModifyState(const M& mod)
{
	return ModifyState([](auto) { return true; }, mod);
}
