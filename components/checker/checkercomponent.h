/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef CHECKERCOMPONENT_H
#define CHECKERCOMPONENT_H

namespace icinga
{

struct ServiceNextCheckLessComparer
{
public:
	bool operator()(Service& a, Service& b)
	{
		return a.GetNextCheck() > b.GetNextCheck();
	}
};

/**
 * @ingroup checker
 */
class CheckerComponent : public Component
{
public:
	typedef shared_ptr<CheckerComponent> Ptr;
	typedef weak_ptr<CheckerComponent> WeakPtr;

	typedef priority_queue<Service, vector<Service>, ServiceNextCheckLessComparer> ServiceQueue;

	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);

private:
	VirtualEndpoint::Ptr m_Endpoint;

	ServiceQueue m_Services;
	set<ConfigObject::Ptr> m_PendingServices;

	Timer::Ptr m_CheckTimer;

	Timer::Ptr m_ResultTimer;

	void CheckTimerHandler(void);
	void ResultTimerHandler(void);

	void CheckCompletedHandler(Service service, const ScriptTask::Ptr& task);

	void AdjustCheckTimer(void);

	void AssignServiceRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request);
	void ClearServicesRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request);
};

}

#endif /* CHECKERCOMPONENT_H */
