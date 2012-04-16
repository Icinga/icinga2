#include "i2-base.h"

using namespace icinga;

CondVar::CondVar(void)
{
#ifdef _WIN32
	InitializeConditionVariable(&m_CondVar);
#else /* _WIN32 */
	
#endif /* _WIN32 */
}

CondVar::~CondVar(void)
{
#ifdef _WIN32
	/* nothing to do here */
#else /* _WIN32 */

#endif /* _WIN32 */
}

void CondVar::Wait(Mutex& mtx)
{
#ifdef _WIN32
	SleepConditionVariableCS(&m_CondVar, mtx.Get(), INFINITE);
#else /* _WIN32 */
	pthread_cond_wait(&m_CondVar, mtx.Get());
#endif /* _WIN32 */
}

void CondVar::Signal(void)
{
#ifdef _WIN32
	WakeConditionVariable(&m_CondVar);
#else /* _WIN32 */
	pthread_cond_signal(&m_CondVar);
#endif /* _WIN32 */
}

void CondVar::Broadcast(void)
{
#ifdef _WIN32
	WakeAllConditionVariable(&m_CondVar);
#else /* _WIN32 */
	pthread_cond_broadcast(&m_CondVar);
#endif /* _WIN32 */
}


#ifdef _WIN32
CONDITION_VARIABLE *CondVar::Get(void)
#else /* _WIN32 */
pthread_cond_t *CondVar::Get(void)
#endif /* _WIN32 */
{
	return &m_CondVar;
}
