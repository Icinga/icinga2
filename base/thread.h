#ifndef THREAD_H
#define THREAD_H

namespace icinga
{

typedef void (*ThreadProc)(void *);

/**
 * Thread
 *
 * A wrapper around OS-specific thread functionality.
 */
class I2_BASE_API Thread
{
private:
#ifdef _WIN32
	HANDLE m_Thread;
#else
	pthread_t m_Thread;
#endif

public:
	Thread(ThreadProc callback);
	~Thread(void);

	void Join(void);
};

}

#endif /* THREAD_H */
