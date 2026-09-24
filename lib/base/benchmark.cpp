// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "base/benchmark.hpp"

using namespace icinga;

void BenchmarkStopWatch::Stop(BenchmarkSummary& summary)
{
	summary.m_Sum.fetch_add((std::chrono::steady_clock::now() - m_Start).count());
}
