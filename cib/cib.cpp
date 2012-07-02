#include "i2-cib.h"

using namespace icinga;

int CIB::m_Types;
Ringbuffer CIB::m_TaskStatistics(15 * 60);

void CIB::RequireInformation(InformationType types)
{
	m_Types |= types;

	Application::Ptr app = Application::GetInstance();
	Component::Ptr component = app->GetComponent("cibsync");

	if (!component) {
		ConfigObject::Ptr cibsyncComponentConfig = boost::make_shared<ConfigObject>("component", "cibsync");
		cibsyncComponentConfig->SetLocal(true);
		cibsyncComponentConfig->Commit();
	}
}

void CIB::UpdateTaskStatistics(long tv, int num)
{
	m_TaskStatistics.InsertValue(tv, num);
}

int CIB::GetTaskStatistics(long timespan)
{
	return m_TaskStatistics.GetValues(timespan);
}

int CIB::GetInformationTypes(void)
{
	return m_Types;
}

