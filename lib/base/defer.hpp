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

	Defer() = default;

	Defer(const Defer&) = delete;
	Defer(Defer&&) = delete;
	Defer& operator=(const Defer&) = delete;
	Defer& operator=(Defer&&) = delete;

	inline
	~Defer()
	{
		if (m_Func) {
			try {
				m_Func();
			} catch (...) {
				// https://stackoverflow.com/questions/130117/throwing-exceptions-out-of-a-destructor
			}
		}
	}

	inline void SetFunc(std::function<void()> func)
	{
		m_Func = std::move(func);
	}

	inline
	void Cancel()
	{
		m_Func = nullptr;
	}

private:
	std::function<void()> m_Func;
};

}

#endif /* DEFER */
