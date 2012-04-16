#ifndef THREAD_H
#define THREAD_H

namespace icinga
{

typedef void (*ThreadProc)(void *);

class I2_BASE_API Thread
{
private:
#ifdef _WIN32
	HANDLE m_Thread;
#else
	pthread_t m_Thread;
#endif

public:
	Thread(void (*callback)(void *));
	~Thread(void);

	void Start(void);
	void Join(void);
};

}

#endif /* THREAD_H */
