/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef UT_CURRENT_H
#define UT_CURRENT_H

#include "base/ut-id.hpp"

namespace icinga
{
namespace UT
{

class Thread;

class Current
{
public:
	static inline
	ID GetID()
	{
		return m_Thread;
	}

	static void Yield_();

	static thread_local Thread* m_Thread;

private:
	inline Current() = default;
};

}
}

#endif /* UT_CURRENT_H */
