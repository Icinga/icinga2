#ifndef TIMER_H
#define TIMER_H

#include <time.h>

namespace icinga {

struct I2_BASE_API TimerEventArgs : public EventArgs
{
	typedef shared_ptr<TimerEventArgs> Ptr;
	typedef weak_ptr<TimerEventArgs> WeakPtr;

	EventArgs::Ptr UserArgs;
};

class I2_BASE_API Timer : public Object
{
private:
	EventArgs::Ptr m_UserArgs;
	unsigned int m_Interval;
	time_t m_Next;

	static time_t NextCall;

	static void RescheduleTimers(void);

	void Call(void);

public:
	typedef shared_ptr<Timer> Ptr;
	typedef weak_ptr<Timer> WeakPtr;

	static list<Timer::WeakPtr> Timers;

	Timer(void);

	void SetInterval(unsigned int interval);
	unsigned int GetInterval(void) const;

	void SetUserArgs(const EventArgs::Ptr& userArgs);
	EventArgs::Ptr GetUserArgs(void) const;

	static time_t GetNextCall(void);
	static void CallExpiredTimers(void);
	static void StopAllTimers(void);

	void Start(void);
	void Stop(void);

	void Reschedule(time_t next);

	event<TimerEventArgs::Ptr> OnTimerExpired;
};

}

#endif /* TIMER_H */
