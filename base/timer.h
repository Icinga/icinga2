#ifndef TIMER_H
#define TIMER_H

#include <time.h>

namespace icinga {

struct TimerEventArgs : public EventArgs
{
	typedef shared_ptr<TimerEventArgs> RefType;
	typedef weak_ptr<TimerEventArgs> WeakRefType;

	EventArgs::RefType UserArgs;
};

class Timer : public Object
{
private:
	EventArgs::RefType m_UserArgs;
	unsigned int m_Interval;
	time_t m_Next;

	static time_t NextCall;

	static void RescheduleTimers(void);

	void Call(void);

public:
	typedef shared_ptr<Timer> RefType;
	typedef weak_ptr<Timer> WeakRefType;

	static list<Timer::WeakRefType> Timers;

	Timer(void);

	void SetInterval(unsigned int interval);
	unsigned int GetInterval(void) const;

	void SetUserArgs(const EventArgs::RefType& userArgs);
	EventArgs::RefType GetUserArgs(void) const;

	static time_t GetNextCall(void);
	static void CallExpiredTimers(void);
	static void StopAllTimers(void);

	void Start(void);
	void Stop(void);

	void Reschedule(time_t next);

	event<TimerEventArgs::RefType> OnTimerExpired;
};

}

#endif /* TIMER_H */
