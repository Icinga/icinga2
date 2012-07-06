#include "i2-cib.h"

using namespace icinga;

int CIB::m_Types;
Ringbuffer CIB::m_TaskStatistics(15 * 60);
boost::signal<void (const ServiceStatusMessage&)> CIB::OnServiceStatusUpdate;

void CIB::RequireInformation(InformationType types)
{
	m_Types |= types;

	Application::Ptr app = Application::GetInstance();
	Component::Ptr component = app->GetComponent("cibsync");

	if (!component) {
		ConfigItemBuilder::Ptr cb = boost::make_shared<ConfigItemBuilder>();
		cb->SetType("component");
		cb->SetName("cibsync");
		cb->SetLocal(true);
		ConfigItem::Ptr ci = cb->Compile();
		ci->Commit();
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

