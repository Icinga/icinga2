/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TIMER_H
#define TIMER_H

#include "base/i2-base.hpp"
#include "base/shared-object.hpp"
#include <boost/signals2.hpp>
#include <memory>

namespace icinga {

/**
 * Abstract interface for providing the current time.
 *
 * This is used to allow mocking of time in unit tests.
 *
 * @ingroup base
 */
class TimeProvider : public SharedObject
{
public:
	/**
	 * Retrieves the current time.
	 *
	 * The time returned by this function might be based on the system clock or a mocked time source.
	 *
	 * @return the current time as a floating point value in seconds with microsecond precision.
	 */
	virtual std::chrono::duration<double> Now() const = 0;
};

class TimerHolder;

/**
 * A timer that periodically triggers an event.
 *
 * @ingroup base
 */
class Timer final
{
public:
	typedef std::shared_ptr<Timer> Ptr;

	static Ptr Create(const intrusive_ptr<TimeProvider const>& timeProvider = nullptr);

	~Timer();

	static void Initialize();
	static void Uninitialize();
	static void InitializeThread();
	static void UninitializeThread();

	void SetInterval(double interval);
	double GetInterval() const;

	static void AdjustTimers(double adjustment);

	void Start();
	void Stop(bool wait = false);

	void Reschedule(double next = -1);
	double GetNext() const;

	boost::signals2::signal<void(const Timer * const&)> OnTimerExpired;

private:
	double m_Interval{0}; /**< The interval of the timer. */
	double m_Next{0}; /**< When the next event should happen. */
	bool m_Started{false}; /**< Whether the timer is enabled. */
	bool m_Running{false}; /**< Whether the timer proc is currently running. */
	intrusive_ptr<TimeProvider const> m_TimeProvider; // The time provider to use.
	std::weak_ptr<Timer> m_Self;

	Timer() = default;
	void Call();
	void InternalReschedule(bool completed, double next = -1);
	void InternalRescheduleUnlocked(bool completed, double next = -1);

	static void TimerThreadProc();

	friend class TimerHolder;
};

}

#endif /* TIMER_H */
