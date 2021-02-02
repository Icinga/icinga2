/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SINGLETON_H
#define SINGLETON_H

#include "base/i2-base.hpp"

namespace icinga
{

/**
 * A singleton.
 *
 * @ingroup base
 */
template<typename T>
class Singleton
{
public:
	static T *GetInstance()
	{
		static T instance;
		return &instance;
	}
};

}

#endif /* SINGLETON_H */
