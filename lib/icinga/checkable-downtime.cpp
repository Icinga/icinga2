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
	return m_DowntimeDepth.load();
}

int Checkable::GetDowntimeDepth() const
{
	return m_DowntimeDepth.load();
}

void Checkable::IncrementDowntimeDepth()
{
	m_DowntimeDepth.fetch_add(1);
}

void Checkable::DecrementDowntimeDepth()
{
	m_DowntimeDepth.fetch_sub(1);
}

std::set<Downtime::Ptr> Checkable::GetDowntimes() const
{
	boost::mutex::scoped_lock lock(m_DowntimeMutex);
	return m_Downtimes;
}

void Checkable::RegisterDowntime(const Downtime::Ptr& downtime)
{
	boost::mutex::scoped_lock lock(m_DowntimeMutex);
	m_Downtimes.insert(downtime);
}

void Checkable::UnregisterDowntime(const Downtime::Ptr& downtime)
{
	boost::mutex::scoped_lock lock(m_DowntimeMutex);
	m_Downtimes.erase(downtime);
}
