/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef INITIALIZE_H
#define INITIALIZE_H

#include "base/i2-base.hpp"
#include <functional>

namespace icinga
{

#define I2_TOKENPASTE(x, y) x ## y
#define I2_TOKENPASTE2(x, y) I2_TOKENPASTE(x, y)

#define I2_UNIQUE_NAME(prefix) I2_TOKENPASTE2(prefix, __COUNTER__)

bool InitializeOnceHelper(const std::function<void()>& func, int priority = 0);

#define INITIALIZE_ONCE(func)									\
	namespace { namespace I2_UNIQUE_NAME(io) {							\
		bool l_InitializeOnce(icinga::InitializeOnceHelper(func));		\
	} }

#define INITIALIZE_ONCE_WITH_PRIORITY(func, priority)						\
	namespace { namespace I2_UNIQUE_NAME(io) {							\
		bool l_InitializeOnce(icinga::InitializeOnceHelper(func, priority));	\
	} }
}

#endif /* INITIALIZE_H */
