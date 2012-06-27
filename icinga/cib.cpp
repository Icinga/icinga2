#include "i2-icinga.h"

using namespace icinga;

int CIB::m_Types;
VirtualEndpoint::Ptr CIB::m_Endpoint;

void CIB::RequireInformation(InformationType types)
{
	m_Types |= types;
}

void CIB::Start(void)
{
	m_Endpoint = boost::make_shared<VirtualEndpoint>();
	if (m_Types & CIB_ServiceStatus) {
		m_Endpoint->RegisterTopicHandler("delegation::ServiceStatus",
		    boost::bind(&CIB::ServiceStatusRequestHandler, _2, _3));
	}
	EndpointManager::GetInstance()->RegisterEndpoint(m_Endpoint);
}

void CIB::ServiceStatusRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
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
}

