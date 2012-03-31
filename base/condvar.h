#ifndef I2_CONDVAR_H
#define I2_CONDVAR_H

namespace icinga
{

class condvar
{
private:
#ifdef _WIN32
	CONDITION_VARIABLE m_CondVar;
#else /* _WIN32 */
	pthread_cond_t m_CondVar;
#endif /* _WIN32 */

public:
	condvar(void);
	~condvar(void);

	void wait(mutex *mtx);
	void signal(void);
	void broadcast(void);

#ifdef _WIN32
	CONDITION_VARIABLE *get(void);
#else /* _WIN32 */
	pthread_cond_t *get(void);
#endif /* _WIN32 */
};

}

#endif /* I2_CONDVAR_H */