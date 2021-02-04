/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/defer.hpp"
#include "base/timer.hpp"
#include "base/debug.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace icinga;

namespace icinga {

class TimerHolder {
public:
	TimerHolder(Timer *timer)
		: m_Timer(timer)
	{ }

	inline Timer *GetObject() const
	{
		return m_Timer;
	}

	inline double GetNextUnlocked() const
	{
		return m_Timer->m_Next;
	}

	operator Timer *() const
	{
		return m_Timer;
	}

private:
	Timer *m_Timer;
};

}

typedef boost::multi_index_container<
	TimerHolder,
	boost::multi_index::indexed_by<
		boost::multi_index::ordered_unique<boost::multi_index::const_mem_fun<TimerHolder, Timer *, &TimerHolder::GetObject> >,
		boost::multi_index::ordered_non_unique<boost::multi_index::const_mem_fun<TimerHolder, double, &TimerHolder::GetNextUnlocked> >
	>
> TimerSet;

static std::mutex l_TimerMutex;
static std::condition_variable l_TimerCV;
static std::thread l_TimerThread;
static bool l_StopTimerThread;
static TimerSet l_Timers;
static int l_AliveTimers = 0;

static Defer l_ShutdownTimersCleanlyOnExit (&Timer::Uninitialize);

/**
 * Destructor for the Timer class.
 */
Timer::~Timer()
{
	Stop(true);
}

void Timer::Initialize()
{
	std::unique_lock<std::mutex> lock(l_TimerMutex);

	if (l_AliveTimers > 0) {
		InitializeThread();
	}
}

void Timer::Uninitialize()
{
	std::unique_lock<std::mutex> lock(l_TimerMutex);

	if (l_AliveTimers > 0) {
		UninitializeThread();
	}
}

void Timer::InitializeThread()
{
	l_StopTimerThread = false;
	l_TimerThread = std::thread(&Timer::TimerThreadProc);
}

void Timer::UninitializeThread()
{
	{
		l_StopTimerThread = true;
		l_TimerCV.notify_all();
	}

	l_TimerMutex.unlock();

	if (l_TimerThread.joinable())
		l_TimerThread.join();

	l_TimerMutex.lock();
}

/**
 * Calls this timer.
 */
void Timer::Call()
{
	try {
		OnTimerExpired(this);
	} catch (...) {
		InternalReschedule(true);

		throw;
	}

	InternalReschedule(true);
}

/**
 * Sets the interval for this timer.
 *
 * @param interval The new interval.
 */
void Timer::SetInterval(double interval)
{
	std::unique_lock<std::mutex> lock(l_TimerMutex);
	m_Interval = interval;
}

/**
 * Retrieves the interval for this timer.
 *
 * @returns The interval.
 */
double Timer::GetInterval() const
{
	std::unique_lock<std::mutex> lock(l_TimerMutex);
	return m_Interval;
}

/**
 * Registers the timer and starts processing events for it.
 */
void Timer::Start()
{
	{
		std::unique_lock<std::mutex> lock(l_TimerMutex);
		m_Started = true;

		if (++l_AliveTimers == 1) {
			InitializeThread();
		}
	}

	InternalReschedule(false);
}

/**
 * Unregisters the timer and stops processing events for it.
 */
void Timer::Stop(bool wait)
{
	if (l_StopTimerThread)
		return;

	std::unique_lock<std::mutex> lock(l_TimerMutex);

	if (m_Started && --l_AliveTimers == 0) {
		UninitializeThread();
	}

	m_Started = false;
	l_Timers.erase(this);

	/* Notify the worker thread that we've disabled a timer. */
	l_TimerCV.notify_all();

	while (wait && m_Running)
		l_TimerCV.wait(lock);
}

void Timer::Reschedule(double next)
{
	InternalReschedule(false, next);
}

/**
 * Reschedules this timer.
 *
 * @param completed Whether the timer has just completed its callback.
 * @param next The time when this timer should be called again. Use -1 to let
 *        the timer figure out a suitable time based on the interval.
 */
void Timer::InternalReschedule(bool completed, double next)
{
	std::unique_lock<std::mutex> lock(l_TimerMutex);

	if (completed)
		m_Running = false;

	if (next < 0) {
		/* Don't schedule the next call if this is not a periodic timer. */
		if (m_Interval <= 0)
			return;

		next = Utility::GetTime() + m_Interval;
	}

	m_Next = next;

	if (m_Started && !m_Running) {
		/* Remove and re-add the timer to update the index. */
		l_Timers.erase(this);
		l_Timers.insert(this);

		/* Notify the worker that we've rescheduled a timer. */
		l_TimerCV.notify_all();
	}
}

/**
 * Retrieves when the timer is next due.
 *
 * @returns The timestamp.
 */
double Timer::GetNext() const
{
	std::unique_lock<std::mutex> lock(l_TimerMutex);
	return m_Next;
}

/**
 * Adjusts all timers by adding the specified amount of time to their
 * next scheduled timestamp.
 *
 * @param adjustment The adjustment.
 */
void Timer::AdjustTimers(double adjustment)
{
	std::unique_lock<std::mutex> lock(l_TimerMutex);

	double now = Utility::GetTime();

	typedef boost::multi_index::nth_index<TimerSet, 1>::type TimerView;
	TimerView& idx = boost::get<1>(l_Timers);

	std::vector<Timer *> timers;

	for (Timer *timer : idx) {
		if (std::fabs(now - (timer->m_Next + adjustment)) <
			std::fabs(now - timer->m_Next)) {
			timer->m_Next += adjustment;
			timers.push_back(timer);
		}
	}

	for (Timer *timer : timers) {
		l_Timers.erase(timer);
		l_Timers.insert(timer);
	}

	/* Notify the worker that we've rescheduled some timers. */
	l_TimerCV.notify_all();
}

/**
 * Worker thread proc for Timer objects.
 */
void Timer::TimerThreadProc()
{
	namespace ch = std::chrono;

	Log(LogDebug, "Timer", "TimerThreadProc started.");

	Utility::SetThreadName("Timer Thread");

	for (;;) {
		std::unique_lock<std::mutex> lock(l_TimerMutex);

		typedef boost::multi_index::nth_index<TimerSet, 1>::type NextTimerView;
		NextTimerView& idx = boost::get<1>(l_Timers);

		/* Wait until there is at least one timer. */
		while (idx.empty() && !l_StopTimerThread)
			l_TimerCV.wait(lock);

		if (l_StopTimerThread)
			break;

		auto it = idx.begin();
		Timer *timer = *it;

		ch::time_point<ch::system_clock, ch::duration<double>> next (ch::duration<double>(timer->m_Next));

		if (next - ch::system_clock::now() > ch::duration<double>(0.01)) {
			/* Wait for the next timer. */
			l_TimerCV.wait_until(lock, next);

			continue;
		}

		/* Remove the timer from the list so it doesn't get called again
		 * until the current call is completed. */
		l_Timers.erase(timer);

		timer->m_Running = true;

		lock.unlock();

		/* Asynchronously call the timer. */
		Utility::QueueAsyncCallback([timer]() { timer->Call(); });
	}
}
