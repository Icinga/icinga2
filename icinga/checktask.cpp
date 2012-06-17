#include "i2-icinga.h"

using namespace icinga;

map<string, CheckTask::Factory> CheckTask::m_Types;

CheckTask::CheckTask(const Service& service)
	: m_Service(service)
{ }

Service CheckTask::GetService(void) const
{
	return m_Service;
}

void CheckTask::RegisterType(string type, Factory factory)
{
	m_Types[type] = factory;
}

CheckTask::Ptr CheckTask::CreateTask(const Service& service)
{
	map<string, CheckTask::Factory>::iterator it;

	it = m_Types.find(service.GetCheckType());

	if (it == m_Types.end())
		throw runtime_error("Invalid check type specified for service '" + service.GetName() + "'");

	return it->second(service);
}
