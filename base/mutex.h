#ifndef MUTEX_H
#define MUTEX_H

namespace icinga
{

/**
 * Mutex
 *
 * A wrapper around OS-specific mutex functionality.
 */
class I2_BASE_API Mutex
{
private:
#ifdef _WIN32
	CRITICAL_SECTION m_Mutex;
#else /* _WIN32 */
	pthread_mutex_t m_Mutex;
#endif /* _WIN32 */

public:
	Mutex(void);
	~Mutex(void);

	bool TryEnter(void);
	void Enter(void);
	void Exit(void);

#ifdef _WIN32
	CRITICAL_SECTION *Get(void);
#else /* _WIN32 */
	pthread_mutex_t *Get(void);
#endif /* _WIN32 */
};

}

#endif /* MUTEX_H */
