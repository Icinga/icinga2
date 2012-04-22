#include "i2-base.h"

using namespace icinga;

typedef struct threadparam_s
{
	ThreadProc callback;
	void *param;
} threadparam_t;

/**
 * ThreadStartProc
 *
 * Helper function that deals with OS-specific differences in the thread
 * proc's function signature.
 */
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

/**
 * Thread
 *
 * Constructor for the thread class. Creates a new thread that begins
 * executing immediately.
 */
Thread::Thread(ThreadProc callback)
{
	threadparam_t *tparam = new threadparam_t();

	if (tparam == NULL)
		throw OutOfMemoryException("Out of memory");

#ifdef _WIN32
	m_Thread = CreateThread(NULL, 0, ThreadStartProc, tparam, CREATE_SUSPENDED, NULL);

	if (m_Thread == NULL)
		throw Win32Exception("CreateThread failed.", GetLastError());
#else /* _WIN32 */
	pthread_create(&m_Thread, NULL, ThreadStartProc, &tparam);
#endif /* _WIN32 */
}

/**
 * ~Thread
 *
 * Destructor for the Thread class. Cleans up the resources associated
 * with the thread.
 */
Thread::~Thread(void)
{
#ifdef _WIN32
	CloseHandle(m_Thread);
#else /* _WIN32 */
	/* nothing to do here */
#endif
}

/**
 * Join
 *
 * Waits until the thread has finished executing.
 */
void Thread::Join(void)
{
#ifdef _WIN32
	WaitForSingleObject(m_Thread, INFINITE);
#else /* _WIN32 */
	pthread_join(m_Thread, NULL);
#endif
}
