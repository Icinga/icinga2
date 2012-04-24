#include "i2-base.h"

using namespace icinga;

/**
 * CondVar
 *
 * Constructor for the CondVar class.
 */
CondVar::CondVar(void)
{
#ifdef _WIN32
	InitializeConditionVariable(&m_CondVar);
#else /* _WIN32 */
	
#endif /* _WIN32 */
}

/**
 * ~CondVar
 *
 * Destructor for the CondVar class.
 */
CondVar::~CondVar(void)
{
#ifdef _WIN32
	/* nothing to do here */
#else /* _WIN32 */

#endif /* _WIN32 */
}

/**
 * Wait
 *
 * Waits for the condition variable to be signaled. Releases the specified mutex
 * before it begins to wait and re-acquires the mutex after waiting.
 *
 * @param mtx The mutex that should be released during waiting.
 */
void CondVar::Wait(Mutex& mtx)
{
#ifdef _WIN32
	SleepConditionVariableCS(&m_CondVar, mtx.Get(), INFINITE);
#else /* _WIN32 */
	pthread_cond_wait(&m_CondVar, mtx.Get());
#endif /* _WIN32 */
}

/**
 * Signal
 *
 * Wakes up at least one waiting thread.
 */
void CondVar::Signal(void)
{
#ifdef _WIN32
	WakeConditionVariable(&m_CondVar);
#else /* _WIN32 */
	pthread_cond_signal(&m_CondVar);
#endif /* _WIN32 */
}

/**
 * Broadcast
 *
 * Wakes up all waiting threads.
 */
void CondVar::Broadcast(void)
{
#ifdef _WIN32
	WakeAllConditionVariable(&m_CondVar);
#else /* _WIN32 */
	pthread_cond_broadcast(&m_CondVar);
#endif /* _WIN32 */
}

/**
 * Get
 *
 * Retrieves the platform-specific condition variable handle.
 *
 * @returns The platform-specific condition variable handle.
 */
#ifdef _WIN32
CONDITION_VARIABLE *CondVar::Get(void)
#else /* _WIN32 */
pthread_cond_t *CondVar::Get(void)
#endif /* _WIN32 */
{
	return &m_CondVar;
}
