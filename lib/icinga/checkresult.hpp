/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CHECKRESULT_H
#define CHECKRESULT_H

#include "base/atomic.hpp"
#include "icinga/i2-icinga.hpp"
#include "icinga/checkresult-ti.hpp"
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <utility>

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

class TestCheckResultProducer : public CheckResultProducer
{
public:
	bool try_lock_shared() noexcept override;
	void unlock_shared() noexcept override;
};

// TODO: revert this before merging!
typedef TestCheckResultProducer TodoCheckResultProducer;

/**
 * A check result.
 *
 * @ingroup icinga
 */
class CheckResult final : public ObjectImpl<CheckResult>
{
public:
	DECLARE_OBJECT(CheckResult);

	CheckResult() : CheckResult(new TodoCheckResultProducer())
	{
	}

	CheckResult(CheckResultProducer::Ptr producer) : m_Producer(std::move(producer))
	{
	}

	const CheckResultProducer::Ptr& GetProducer() const noexcept
	{
		return m_Producer;
	}

	double CalculateExecutionTime() const;
	double CalculateLatency() const;

private:
	CheckResultProducer::Ptr m_Producer;
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
		uint32_t m_InstanceIsActive = 0;
		uint32_t m_ProcessingCheckResults = 0;
	};

	Atomic<State> m_State {State{}};
	std::mutex m_Mutex;
	std::condition_variable m_CV;

	template<class F>
	State ModifyState(const F& func)
	{
		auto expected (m_State.load());
		decltype(expected) desired;

		do {
			desired = expected;
			func(desired);
		} while (!m_State.compare_exchange_weak(expected, desired));

		return desired;
	}
};

template<>
struct TypeHelper<CheckResult, false>
{
	static ObjectFactory GetFactory();

private:
	static Object::Ptr Factory(const std::vector<Value>&);
};

class DslCheckResultProducer : public CheckResultProducer
{
	friend TypeHelper<CheckResult, false>;

public:
	bool try_lock_shared() noexcept override;
	void unlock_shared() noexcept override;

private:
	static Ptr m_Instance;
};

}

#endif /* CHECKRESULT_H */
