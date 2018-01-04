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

#include "icinga/checkable.hpp"
#include "icinga/checkable.tcpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(Checkable, Checkable::GetPrototype());
INITIALIZE_ONCE(&Checkable::StaticInitialize);

boost::signals2::signal<void (const Checkable::Ptr&, const String&, const String&, AcknowledgementType, bool, bool, double, const MessageOrigin::Ptr&)> Checkable::OnAcknowledgementSet;
boost::signals2::signal<void (const Checkable::Ptr&, const MessageOrigin::Ptr&)> Checkable::OnAcknowledgementCleared;

void Checkable::StaticInitialize()
{
	/* fixed downtime start */
	Downtime::OnDowntimeStarted.connect(std::bind(&Checkable::NotifyFixedDowntimeStart, _1));
	/* flexible downtime start */
	Downtime::OnDowntimeTriggered.connect(std::bind(&Checkable::NotifyFlexibleDowntimeStart, _1));
	/* fixed/flexible downtime end */
	Downtime::OnDowntimeRemoved.connect(std::bind(&Checkable::NotifyDowntimeEnd, _1));
}

Checkable::Checkable()
	: m_CheckRunning(false)
{
	SetSchedulingOffset(Utility::Random());
}

void Checkable::OnAllConfigLoaded()
{
	ObjectImpl<Checkable>::OnAllConfigLoaded();

	Endpoint::Ptr endpoint = GetCommandEndpoint();

	if (endpoint) {
		Zone::Ptr checkableZone = static_pointer_cast<Zone>(GetZone());

		if (!checkableZone)
			checkableZone = Zone::GetLocalZone();

		Zone::Ptr cmdZone = endpoint->GetZone();

		if (checkableZone && cmdZone != checkableZone && cmdZone->GetParent() != checkableZone) {
			BOOST_THROW_EXCEPTION(ValidationError(this, { "command_endpoint" },
				"Command endpoint must be in zone '" + checkableZone->GetName() + "' or in a direct child zone thereof."));
		}
	}
}

void Checkable::Start(bool runtimeCreated)
{
	double now = Utility::GetTime();

	if (GetNextCheck() < now + 300)
		UpdateNextCheck();

	ObjectImpl<Checkable>::Start(runtimeCreated);
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

AcknowledgementType Checkable::GetAcknowledgement()
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

bool Checkable::IsAcknowledged() const
{
	return const_cast<Checkable *>(this)->GetAcknowledgement() != AcknowledgementNone;
}

void Checkable::AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, bool notify, bool persistent, double expiry, const MessageOrigin::Ptr& origin)
{
	SetAcknowledgementRaw(type);
	SetAcknowledgementExpiry(expiry);

	if (notify && !IsPaused())
		OnNotificationsRequested(this, NotificationAcknowledgement, GetLastCheckResult(), author, comment, nullptr);

	OnAcknowledgementSet(this, author, comment, type, notify, persistent, expiry, origin);
}

void Checkable::ClearAcknowledgement(const MessageOrigin::Ptr& origin)
{
	SetAcknowledgementRaw(AcknowledgementNone);
	SetAcknowledgementExpiry(0);

	OnAcknowledgementCleared(this, origin);
}

Endpoint::Ptr Checkable::GetCommandEndpoint() const
{
	return Endpoint::GetByName(GetCommandEndpointRaw());
}

int Checkable::GetSeverity() const
{
	/* overridden in Host/Service class. */
	return 0;
}

void Checkable::NotifyFixedDowntimeStart(const Downtime::Ptr& downtime)
{
	if (!downtime->GetFixed())
		return;

	NotifyDowntimeInternal(downtime);
}

void Checkable::NotifyFlexibleDowntimeStart(const Downtime::Ptr& downtime)
{
	if (downtime->GetFixed())
		return;

	NotifyDowntimeInternal(downtime);
}

void Checkable::NotifyDowntimeInternal(const Downtime::Ptr& downtime)
{
	Checkable::Ptr checkable = downtime->GetCheckable();

	if (!checkable->IsPaused())
		OnNotificationsRequested(checkable, NotificationDowntimeStart, checkable->GetLastCheckResult(), downtime->GetAuthor(), downtime->GetComment(), nullptr);
}

void Checkable::NotifyDowntimeEnd(const Downtime::Ptr& downtime)
{
	/* don't send notifications for flexible downtimes which never triggered */
	if (!downtime->GetFixed() && !downtime->IsTriggered())
		return;

	Checkable::Ptr checkable = downtime->GetCheckable();

	if (!checkable->IsPaused())
		OnNotificationsRequested(checkable, NotificationDowntimeEnd, checkable->GetLastCheckResult(), downtime->GetAuthor(), downtime->GetComment(), nullptr);
}

void Checkable::ValidateCheckInterval(double value, const ValidationUtils& utils)
{
	ObjectImpl<Checkable>::ValidateCheckInterval(value, utils);

	if (value <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "check_interval" }, "Interval must be greater than 0."));
}

void Checkable::ValidateMaxCheckAttempts(int value, const ValidationUtils& utils)
{
	ObjectImpl<Checkable>::ValidateMaxCheckAttempts(value, utils);

	if (value <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "max_check_attempts" }, "Value must be greater than 0."));
}
