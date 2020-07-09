// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base/atomic.hpp"
#include <chrono>
#include <ratio>

namespace icinga
{

class BenchmarkSummary;

/**
 * Stopwatch for benchmarking.
 *
 * @ingroup base
 */
class BenchmarkStopWatch {
public:
	void Start()
	{
		m_Start = std::chrono::steady_clock::now();
	}

	void Stop(BenchmarkSummary& summary);

private:
	decltype(std::chrono::steady_clock::now()) m_Start;
};

/**
 * Benchmark result.
 *
 * @ingroup base
 */
class BenchmarkSummary
{
	friend BenchmarkStopWatch;

public:
	BenchmarkSummary() : m_Sum(0)
	{
	}

	long double GetSeconds() const
	{
		typedef std::ratio_divide<std::chrono::steady_clock::duration::period, std::chrono::seconds::period> factor;
		return static_cast<long double>(m_Sum.load()) * factor::num / factor::den;
	}

private:
	Atomic<std::chrono::steady_clock::duration::rep> m_Sum;
};

}
