#ifndef THREAD_H
#define THREAD_H

namespace icinga
{

class thread
{
private:
#ifdef _WIN32
	HANDLE m_Thread;
#else
	pthread_t m_Thread;
#endif

public:
	thread(void (*callback)(void *));
	~thread(void);

	void start(void);
	void terminate(void);
	void join(void);
};

}

#endif /* THREAD_H */
