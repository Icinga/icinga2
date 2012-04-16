#include "i2-base.h"

using namespace icinga;

Mutex::Mutex(void)
{
#ifdef _WIN32
	InitializeCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_init(&m_Mutex, NULL);
#endif /* _WIN32 */
}

Mutex::~Mutex(void)
{
#ifdef _WIN32
	DeleteCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_destroy(&m_Mutex);
#endif /* _WIN32 */
}

bool Mutex::TryEnter(void)
{
#ifdef _WIN32
	return (TryEnterCriticalSection(&m_Mutex) == TRUE);
#else /* _WIN32 */
	return pthread_mutex_trylock(&m_Mutex);
#endif /* _WIN32 */
}

void Mutex::Enter(void)
{
#ifdef _WIN32
	EnterCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_lock(&m_Mutex);
#endif /* _WIN32 */
}

void Mutex::Exit(void)
{
#ifdef _WIN32
	LeaveCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_unlock(&m_Mutex);
#endif /* _WIN32 */
}

#ifdef _WIN32
CRITICAL_SECTION *Mutex::Get(void)
#else /* _WIN32 */
pthread_mutex_t *Mutex::Get(void)
#endif /* _WIN32 */
{
	return &m_Mutex;
}
