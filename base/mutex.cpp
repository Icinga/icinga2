#include "i2-base.h"

using namespace icinga;

mutex::mutex(void)
{
#ifdef _WIN32
	InitializeCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_init(&m_Mutex, NULL);
#endif /* _WIN32 */
}

mutex::~mutex(void)
{
#ifdef _WIN32
	DeleteCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_destroy(&m_Mutex);
#endif /* _WIN32 */
}

bool mutex::tryenter(void)
{
#ifdef _WIN32
	return (TryEnterCriticalSection(&m_Mutex) == TRUE);
#else /* _WIN32 */
	return pthread_mutex_trylock(&m_Mutex);
#endif /* _WIN32 */
}

void mutex::enter(void)
{
#ifdef _WIN32
	EnterCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_lock(&m_Mutex);
#endif /* _WIN32 */
}

void mutex::exit(void)
{
#ifdef _WIN32
	LeaveCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_unlock(&m_Mutex);
#endif /* _WIN32 */
}

#ifdef _WIN32
CRITICAL_SECTION *mutex::get(void)
#else /* _WIN32 */
pthread_mutex_t *mutex::get(void)
#endif /* _WIN32 */
{
	return &m_Mutex;
}
