/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/atomic.hpp"
#include "base/object.hpp"
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace icinga
{

/**
 * A component that produces check results and hence is responsible for them, e.g. CheckerComponent.
 * I.e. on its shutdown it has to clean up and/or wait for its own ongoing checks if any.
 *
 * @ingroup icinga
 */
class CheckResultProducer : virtual public Object
{
public:
	DECLARE_PTR_TYPEDEFS(CheckResultProducer);

	/**
	 * Requests to delay the producer shutdown (if any) for a CheckResult to be processed.
	 *
	 * @return Whether request was accepted.
	 */
	virtual bool try_lock_shared() noexcept = 0;

	/**
	 * Releases one semaphore slot acquired for CheckResult processing.
	 */
	virtual void unlock_shared() noexcept = 0;
};

class UnitTestCRP : public CheckResultProducer
{
public:
	bool try_lock_shared() noexcept override;
	void unlock_shared() noexcept override;
};

class CheckResultProducerComponent : public CheckResultProducer
{
public:
	bool try_lock_shared() noexcept override;
	void unlock_shared() noexcept override;

protected:
	void Start();
	void Stop();

private:
	struct State
	{
		uint32_t InstanceIsActive = 0;
		uint32_t ProcessingCheckResults = 0;
	};

	Atomic<State> m_State {State{}};
	std::mutex m_Mutex;
	std::condition_variable m_CV;

	template<class C, class M>
	State ModifyState(const C& cond, const M& mod);

	template<class M>
	State ModifyState(const M& mod);
};

}
