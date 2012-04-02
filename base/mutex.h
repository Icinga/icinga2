#ifndef MUTEX_H
#define MUTEX_H

namespace icinga
{

class mutex
{
private:
#ifdef _WIN32
	CRITICAL_SECTION m_Mutex;
#else /* _WIN32 */
	pthread_mutex_t m_Mutex;
#endif /* _WIN32 */

public:
	mutex(void);
	~mutex(void);

	bool tryenter(void);
	void enter(void);
	void exit(void);

#ifdef _WIN32
	CRITICAL_SECTION *get(void);
#else /* _WIN32 */
	pthread_mutex_t *get(void);
#endif /* _WIN32 */
};

}

#endif /* MUTEX_H */
