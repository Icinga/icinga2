#include "i2-icinga.h"

using namespace icinga;

map<string, CheckTaskType> CheckTask::m_Types;

CheckTask::CheckTask(const Service& service)
	: m_Service(service)
{ }

Service CheckTask::GetService(void) const
{
	return m_Service;
}

void CheckTask::RegisterType(string type, Factory factory, QueueFlusher qflusher)
{
	CheckTaskType ctt;
	ctt.Factory = factory;
	ctt.QueueFlusher = qflusher;

	m_Types[type] = ctt;
}

CheckTask::Ptr CheckTask::CreateTask(const Service& service)
{
	map<string, CheckTaskType>::iterator it;

	it = m_Types.find(service.GetCheckType());

	if (it == m_Types.end())
		throw runtime_error("Invalid check type specified for service '" + service.GetName() + "'");

	return it->second.Factory(service);
}

void CheckTask::Enqueue(const CheckTask::Ptr& task)
{
	task->Enqueue();
}

void CheckTask::FlushQueue(void)
{
	map<string, CheckTaskType>::iterator it;

	for (it = m_Types.begin(); it != m_Types.end(); it++)
		it->second.QueueFlusher();
}
