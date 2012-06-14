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
	bool operator()(const Service& a, const Service& b)
	{
		return a.GetNextCheck() < b.GetNextCheck();
	}
};

/**
 * @ingroup checker
 */
class CheckerComponent : public IcingaComponent
{
public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);

private:
	priority_queue<Service, vector<Service>, ServiceNextCheckLessComparer> m_Services;
	Timer::Ptr m_CheckTimer;
	VirtualEndpoint::Ptr m_CheckerEndpoint;

	int CheckTimerHandler(const TimerEventArgs& ea);

	int AssignServiceRequestHandler(const NewRequestEventArgs& nrea);
	int RevokeServiceRequestHandler(const NewRequestEventArgs& nrea);
};

}

#endif /* CHECKERCOMPONENT_H */
