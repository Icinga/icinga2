/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DEFER
#define DEFER

#include <functional>
#include <utility>

namespace icinga
{

/**
 * An action to be executed at end of scope.
 *
 * @ingroup base
 */
class Defer
{
public:
	inline
	Defer(std::function<void()> func) : m_Func(std::move(func))
	{
	}

	Defer(const Defer&) = delete;
	Defer(Defer&&) = delete;
	Defer& operator=(const Defer&) = delete;
	Defer& operator=(Defer&&) = delete;

	inline
	~Defer()
	{
		m_Func();
	}

private:
	std::function<void()> m_Func;
};

}

#endif /* DEFER */
