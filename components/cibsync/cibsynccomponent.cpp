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

#include "i2-cibsync.h"

using namespace icinga;

/**
 * Returns the name of the component.
 *
 * @returns The name.
 */
string CIBSyncComponent::GetName(void) const
{
	return "cibsync";
}

/**
 * Starts the component.
 */
void CIBSyncComponent::Start(void)
{
	m_Endpoint = boost::make_shared<VirtualEndpoint>();
	m_Endpoint->RegisterTopicHandler("delegation::ServiceStatus",
	    boost::bind(&CIBSyncComponent::ServiceStatusRequestHandler, _2, _3));
	EndpointManager::GetInstance()->RegisterEndpoint(m_Endpoint);
}

/**
 * Stops the component.
 */
void CIBSyncComponent::Stop(void)
{
	EndpointManager::Ptr endpointManager = EndpointManager::GetInstance();

	if (endpointManager)
		endpointManager->UnregisterEndpoint(m_Endpoint);
}

void CIBSyncComponent::ServiceStatusRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	string svcname;
	if (!params.GetProperty("service", &svcname))
		return;

	Service service = Service::GetByName(svcname);

	long nextCheck;
	if (params.GetProperty("next_check", &nextCheck))
		service.SetNextCheck(nextCheck);

	long state, stateType;
	if (params.GetProperty("state", &state) && params.GetProperty("state_type", &stateType)) {
		long old_state, old_stateType;
		old_state = service.GetState();
		old_stateType = service.GetStateType();

		if (state != old_state) {
			time_t now;
			time(&now);

			service.SetLastStateChange(now);

			if (old_stateType != stateType)
				service.SetLastHardStateChange(now);
		}

		service.SetState(static_cast<ServiceState>(state));
		service.SetStateType(static_cast<ServiceStateType>(stateType));
	}

	long attempt;
	if (params.GetProperty("current_attempt", &attempt))
		service.SetCurrentCheckAttempt(attempt);

	Dictionary::Ptr cr;
	if (params.GetProperty("result", &cr))
		service.SetLastCheckResult(cr);

	time_t now;
	time(&now);
	CIB::UpdateTaskStatistics(now, 1);
}

EXPORT_COMPONENT(cibsync, CIBSyncComponent);
