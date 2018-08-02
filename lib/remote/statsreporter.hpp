/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef STATSREPORTER_H
#define STATSREPORTER_H

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

#endif /* STATSREPORTER_H */
