/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TIMER_H
#define TIMER_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <boost/signals2.hpp>

namespace icinga {

class TimerHolder;

/**
 * A timer that periodically triggers an event.
 *
 * @ingroup base
 */
class Timer final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Timer);

	~Timer() override;

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

	void Call();
	void InternalReschedule(bool completed, double next = -1);

	static void TimerThreadProc();

	friend class TimerHolder;
};

}

#endif /* TIMER_H */
