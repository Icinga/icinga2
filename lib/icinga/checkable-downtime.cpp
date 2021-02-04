/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"

using namespace icinga;

void Checkable::RemoveAllDowntimes()
{
	for (const Downtime::Ptr& downtime : GetDowntimes()) {
		Downtime::RemoveDowntime(downtime->GetName(), true, true);
	}
}

void Checkable::TriggerDowntimes()
{
	for (const Downtime::Ptr& downtime : GetDowntimes()) {
		downtime->TriggerDowntime();
	}
}

bool Checkable::IsInDowntime() const
{
	for (const Downtime::Ptr& downtime : GetDowntimes()) {
		if (downtime->IsInEffect())
			return true;
	}

	return false;
}

int Checkable::GetDowntimeDepth() const
{
	int downtime_depth = 0;

	for (const Downtime::Ptr& downtime : GetDowntimes()) {
		if (downtime->IsInEffect())
			downtime_depth++;
	}

	return downtime_depth;
}

std::set<Downtime::Ptr> Checkable::GetDowntimes() const
{
	std::unique_lock<std::mutex> lock(m_DowntimeMutex);
	return m_Downtimes;
}

void Checkable::RegisterDowntime(const Downtime::Ptr& downtime)
{
	std::unique_lock<std::mutex> lock(m_DowntimeMutex);
	m_Downtimes.insert(downtime);
}

void Checkable::UnregisterDowntime(const Downtime::Ptr& downtime)
{
	std::unique_lock<std::mutex> lock(m_DowntimeMutex);
	m_Downtimes.erase(downtime);
}
