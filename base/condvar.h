#ifndef CONDVAR_H
#define CONDVAR_H

namespace icinga
{

/**
 * CondVar
 *
 * A wrapper around OS-specific condition variable functionality.
 */
class I2_BASE_API CondVar
{
private:
#ifdef _WIN32
	CONDITION_VARIABLE m_CondVar;
#else /* _WIN32 */
	pthread_cond_t m_CondVar;
#endif /* _WIN32 */

public:
	CondVar(void);
	~CondVar(void);

	void Wait(Mutex& mtx);
	void Signal(void);
	void Broadcast(void);

#ifdef _WIN32
	CONDITION_VARIABLE *Get(void);
#else /* _WIN32 */
	pthread_cond_t *Get(void);
#endif /* _WIN32 */
};

}

#endif /* CONDVAR_H */
