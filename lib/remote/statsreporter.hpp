// SPDX-FileCopyrightText: 2018 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/string.hpp"
#include "base/timer.hpp"
#include "base/value.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include <atomic>
#include <boost/thread/mutex.hpp>
#include <map>

namespace icinga
{

/**
* @ingroup remote
*/
class StatsReporter
{
public:
	static Value ClusterStatsAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

private:
	StatsReporter();

	void OnConnected(const Endpoint::Ptr& endpoint);
	void ReportStats();
	Dictionary::Ptr GenerateStats();
	void ClusterStatsHandler(const String& endpoint, const Dictionary::Ptr& stats);

	static StatsReporter m_Instance;

	std::atomic_flag m_HasBeenInitialized = ATOMIC_FLAG_INIT;
	Timer::Ptr timer;

	boost::mutex m_Mutex;
	std::map<String, Dictionary::Ptr> m_SecondaryStats;
};

}
