#include "i2-base.h"

using namespace icinga;

typedef struct threadparam_s
{
	void (*callback)(void*);
	void *param;
} threadparam_t;

#ifdef _WIN32
static DWORD WINAPI ThreadStartProc(LPVOID param)
{
	threadparam_t *tparam = (threadparam_t *)param;
	tparam->callback(tparam->param);
	delete tparam;
	return 0;
}
#else /* _WIN32 */
static void *ThreadStartProc(void *param)
{
	threadparam_t *tparam = (threadparam_t *)param;
	tparam->callback(tparam->param);
	delete tparam;
	return NULL;
}
#endif /* _WIN32 */


thread::thread(void (*callback)(void *))
{
	threadparam_t *tparam = new threadparam_t();

	if (tparam == NULL)
		throw exception(/*"Out of memory"*/);

#ifdef _WIN32
	m_Thread = CreateThread(NULL, 0, ThreadStartProc, tparam, CREATE_SUSPENDED, NULL);
#else /* _WIN32 */
	pthread_create(&m_Thread, NULL, ThreadStartProc, &tparam);
#endif /* _WIN32 */
}

thread::~thread(void)
{
#ifdef _WIN32
	CloseHandle(m_Thread);
#else /* _WIN32 */
	/* nothing to do here */
#endif
}
	
void thread::terminate(void)
{
#ifdef _WIN32
	TerminateThread(m_Thread, 0);
#else /* _WIN32 */
	/* nothing to do here */
#endif
}

void thread::join(void)
{
#ifdef _WIN32
	WaitForSingleObject(m_Thread, INFINITE);
#else /* _WIN32 */
	pthread_join(m_Thread, NULL);
#endif
}