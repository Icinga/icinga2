#include "i2-base.h"

using namespace icinga;

condvar::condvar(void)
{
#ifdef _WIN32
	InitializeConditionVariable(&m_CondVar);
#else /* _WIN32 */
	
#endif /* _WIN32 */
}

condvar::~condvar(void)
{
#ifdef _WIN32
	/* nothing to do here */
#else /* _WIN32 */

#endif /* _WIN32 */
}

void condvar::wait(mutex *mtx)
{
#ifdef _WIN32
	SleepConditionVariableCS(&m_CondVar, mtx->get(), INFINITE);
#else /* _WIN32 */
	pthread_cond_wait(&m_CondVar, mtx->get());
#endif /* _WIN32 */
}

void condvar::signal(void)
{
#ifdef _WIN32
	WakeConditionVariable(&m_CondVar);
#else /* _WIN32 */
	pthread_cond_signal(&m_CondVar);
#endif /* _WIN32 */
}

void condvar::broadcast(void)
{
#ifdef _WIN32
	WakeAllConditionVariable(&m_CondVar);
#else /* _WIN32 */
	pthread_cond_broadcast(&m_CondVar);
#endif /* _WIN32 */
}


#ifdef _WIN32
CONDITION_VARIABLE *condvar::get(void)
#else /* _WIN32 */
pthread_cond_t *condvar::get(void)
#endif /* _WIN32 */
{
	return &m_CondVar;
}
