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

#ifndef CIB_H
#define CIB_H

#include "icinga/i2-icinga.hpp"
#include "base/ringbuffer.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"

namespace icinga
{

struct CheckableCheckStatistics {
	double min_latency;
	double max_latency;
	double avg_latency;
	double min_execution_time;
	double max_execution_time;
	double avg_execution_time;
};

struct ServiceStatistics {
	double services_ok;
	double services_warning;
	double services_critical;
	double services_unknown;
	double services_pending;
	double services_unreachable;
	double services_flapping;
	double services_in_downtime;
	double services_acknowledged;
};

struct HostStatistics {
	double hosts_up;
	double hosts_down;
	double hosts_unreachable;
	double hosts_pending;
	double hosts_flapping;
	double hosts_in_downtime;
	double hosts_acknowledged;
};

/**
 * Common Information Base class. Holds some statistics (and will likely be
 * removed/refactored).
 *
 * @ingroup icinga
 */
class CIB
{
public:
	static void UpdateActiveHostChecksStatistics(long tv, int num);
	static int GetActiveHostChecksStatistics(long timespan);

	static void UpdateActiveServiceChecksStatistics(long tv, int num);
	static int GetActiveServiceChecksStatistics(long timespan);

	static void UpdatePassiveHostChecksStatistics(long tv, int num);
	static int GetPassiveHostChecksStatistics(long timespan);

	static void UpdatePassiveServiceChecksStatistics(long tv, int num);
	static int GetPassiveServiceChecksStatistics(long timespan);

	static CheckableCheckStatistics CalculateHostCheckStats();
	static CheckableCheckStatistics CalculateServiceCheckStats();
	static HostStatistics CalculateHostStats();
	static ServiceStatistics CalculateServiceStats();

	static std::pair<Dictionary::Ptr, Array::Ptr> GetFeatureStats();

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

private:
	CIB();

	static boost::mutex m_Mutex;
	static RingBuffer m_ActiveHostChecksStatistics;
	static RingBuffer m_PassiveHostChecksStatistics;
	static RingBuffer m_ActiveServiceChecksStatistics;
	static RingBuffer m_PassiveServiceChecksStatistics;
};

}

#endif /* CIB_H */
