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

#include "base/application.hpp"
#include "base/array.hpp"
#include "base/configtype.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/objectlock.hpp"
#include "base/scriptglobal.hpp"
#include "base/utility.hpp"
#include "base/value.hpp"
#include "remote/apifunction.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include "remote/statsreporter.hpp"
#include "remote/zone.hpp"
#include <boost/thread/mutex.hpp>
#include <set>

using namespace icinga;

REGISTER_APIFUNCTION(ClusterStats, event, &StatsReporter::ClusterStatsAPIHandler);

StatsReporter StatsReporter::m_Instance;

StatsReporter::StatsReporter()
{
	Endpoint::OnConnected.connect([this](const Endpoint::Ptr& endpoint, const intrusive_ptr<JsonRpcConnection>&) {
		OnConnected(endpoint);
	});
}

void StatsReporter::OnConnected(const Endpoint::Ptr& endpoint)
{
	if (!m_HasBeenInitialized.test_and_set()) {
		timer = Timer::Create();
		timer->OnTimerExpired.connect([this](const Timer * const&) { ReportStats(); });
		timer->SetInterval(10);
		timer->Start();
		timer->Reschedule(1);
	}
}

void StatsReporter::ReportStats()
{
	auto parent (Zone::GetLocalZone()->GetParent());

	if (parent) {
		Dictionary::Ptr message;

		for (auto& endpoint : parent->GetEndpoints()) {
			for (auto& client : endpoint->GetClients()) {
				if (!message) {
					message = new Dictionary({
						{ "jsonrpc", "2.0" },
						{ "method", "event::ClusterStats" },
						{ "params", new Dictionary({
							{ "stats", GenerateStats() }
						}) }
					});
				}

				client->SendMessage(message);

				break;
			}
		}
	}
}

Dictionary::Ptr StatsReporter::GenerateStats()
{
	auto allStats (new Dictionary);

	{
		boost::mutex::scoped_lock lock (m_Mutex);

		for (auto& endpointStats : m_SecondaryStats) {
			allStats->Set(endpointStats.first, endpointStats.second);
		}
	}

	auto localZone (Zone::GetLocalZone());
	auto parentZone (localZone->GetParent());
	auto unorderedZones (ConfigType::GetObjectsByType<Zone>());
	std::set<Zone::Ptr> zones (unorderedZones.begin(), unorderedZones.end());
	std::set<Endpoint::Ptr> endpoints;
	auto ourStatus (new Dictionary);
	auto now (Utility::GetTime());

	unorderedZones.clear();

	for (auto zone (zones.begin()); zone != zones.end();) {
		if ((*zone)->GetParent() == localZone) {
			++zone;
		} else {
			zones.erase(zone++);
		}
	}

	zones.emplace(localZone);

	if (parentZone)
		zones.emplace(parentZone);

	for (auto& zone : zones) {
		auto zoneEndpoints (zone->GetEndpoints());
		endpoints.insert(zoneEndpoints.begin(), zoneEndpoints.end());
	}

	endpoints.erase(Endpoint::GetLocalEndpoint());

	for (auto& endpoint : endpoints) {
		ourStatus->Set(endpoint->GetName(), new Dictionary({
			{"connected", endpoint->GetConnected()},
			{"last_message_received", endpoint->GetLastMessageReceived()}
		}));
	}

	allStats->Set(Endpoint::GetLocalEndpoint()->GetName(), new Dictionary({
		{"mtime", now},
		{"version", Application::GetAppVersion()},
		{"uptime", now - Application::GetStartTime()},
		{"endpoints", ourStatus}
	}));

	return allStats;
}

Value StatsReporter::ClusterStatsAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	auto endpoint (origin->FromClient->GetEndpoint());

	if (endpoint && endpoint->GetZone()->IsChildOf(Zone::GetLocalZone())) {
		auto rawStats (params->Get("stats"));

		if (rawStats.IsObject()) {
			Dictionary::Ptr allStats (rawStats);

			if (allStats) {
				// Don't permit any child to speak in our zone's name
				std::set<String> neighborhood;

				for (auto& endpoint : Zone::GetLocalZone()->GetEndpoints()) {
					neighborhood.emplace(endpoint->GetName());
				}

				ObjectLock lock (allStats);

				for (auto& endpointStats : allStats) {
					if (endpointStats.second.IsObject()) {
						Dictionary::Ptr stats (endpointStats.second);

						if (stats && neighborhood.find(endpointStats.first) == neighborhood.end()) {
							m_Instance.ClusterStatsHandler(endpointStats.first, endpointStats.second);
						}
					}
				}
			}
		}
	}

	return Empty;
}

void StatsReporter::ClusterStatsHandler(const String& endpoint, const Dictionary::Ptr& stats)
{
	boost::mutex::scoped_lock lock (m_Mutex);
	m_SecondaryStats[endpoint] = stats;
}
