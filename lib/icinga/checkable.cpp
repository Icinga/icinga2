/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/checkable.hpp"
#include "icinga/checkable.tcpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>
#include <boost/bind/apply.hpp>

using namespace icinga;

REGISTER_TYPE(Checkable);

boost::signals2::signal<void (const Checkable::Ptr&, const String&, const String&, AcknowledgementType, bool, double, const MessageOrigin::Ptr&)> Checkable::OnAcknowledgementSet;
boost::signals2::signal<void (const Checkable::Ptr&, const MessageOrigin::Ptr&)> Checkable::OnAcknowledgementCleared;

Checkable::Checkable(void)
	: m_CheckRunning(false)
{
	SetSchedulingOffset(Utility::Random());
}

void Checkable::Start(void)
{
	double now = Utility::GetTime();

	if (GetNextCheck() < now + 300)
		UpdateNextCheck();

	ObjectImpl<Checkable>::Start();
}

void Checkable::OnStateLoaded(void)
{
	AddDowntimesToCache();
	AddCommentsToCache();

	std::vector<String> ids;
	Dictionary::Ptr downtimes = GetDowntimes();

	{
		ObjectLock dlock(downtimes);
		BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
			Downtime::Ptr downtime = kv.second;

			if (downtime->GetScheduledBy().IsEmpty())
				continue;

			ids.push_back(kv.first);
		}
	}

	BOOST_FOREACH(const String& id, ids) {
		/* override config owner to clear downtimes once */
		Downtime::Ptr downtime = GetDowntimeByID(id);
		downtime->SetConfigOwner(Empty);
		RemoveDowntime(id, true);
	}
}

void Checkable::AddGroup(const String& name)
{
	boost::mutex::scoped_lock lock(m_CheckableMutex);

	Array::Ptr groups;
	Host *host = dynamic_cast<Host *>(this);

	if (host)
		groups = host->GetGroups();
	else
		groups = static_cast<Service *>(this)->GetGroups();

	if (groups && groups->Contains(name))
		return;

	if (!groups)
		groups = new Array();

	groups->Add(name);
}

AcknowledgementType Checkable::GetAcknowledgement(void)
{
	AcknowledgementType avalue = static_cast<AcknowledgementType>(GetAcknowledgementRaw());

	if (avalue != AcknowledgementNone) {
		double expiry = GetAcknowledgementExpiry();

		if (expiry != 0 && expiry < Utility::GetTime()) {
			avalue = AcknowledgementNone;
			ClearAcknowledgement();
		}
	}

	return avalue;
}

bool Checkable::IsAcknowledged(void)
{
	return GetAcknowledgement() != AcknowledgementNone;
}

void Checkable::AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, bool notify, double expiry, const MessageOrigin::Ptr& origin)
{
	SetAcknowledgementRaw(type);
	SetAcknowledgementExpiry(expiry);

	if (notify)
		OnNotificationsRequested(this, NotificationAcknowledgement, GetLastCheckResult(), author, comment);

	OnAcknowledgementSet(this, author, comment, type, notify, expiry, origin);
}

void Checkable::ClearAcknowledgement(const MessageOrigin::Ptr& origin)
{
	SetAcknowledgementRaw(AcknowledgementNone);
	SetAcknowledgementExpiry(0);

	OnAcknowledgementCleared(this, origin);
}

Endpoint::Ptr Checkable::GetCommandEndpoint(void) const
{
	return Endpoint::GetByName(GetCommandEndpointRaw());
}

void Checkable::ValidateCheckInterval(double value, const ValidationUtils& utils)
{
	ObjectImpl<Checkable>::ValidateCheckInterval(value, utils);

	if (value <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("check_interval"), "Interval must be greater than 0."));
}
