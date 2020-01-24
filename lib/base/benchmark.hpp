// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <chrono>

namespace icinga
{

/**
 * A stopwatch.
 *
 * @ingroup base
 */
template<class Clock = std::chrono::steady_clock>
class Benchmark final
{
public:
	inline void Start()
	{
		m_Start = Clock::now();
	}

	inline double Stop()
	{
		return std::chrono::duration<double>((Clock::now() - m_Start)).count();
	}

private:
	typename Clock::time_point m_Start;
};

}

#endif /* BENCHMARK_H */
