#include "i2-icinga.h"

using namespace icinga;

map<string, CheckTaskType> CheckTask::m_Types;
vector<CheckTask::Ptr> CheckTask::m_FinishedTasks;
mutex CheckTask::m_FinishedTasksMutex;

CheckTask::CheckTask(const Service& service)
	: m_Service(service)
{ }

Service& CheckTask::GetService(void)
{
	return m_Service;
}

CheckResult& CheckTask::GetResult(void)
{
	return m_Result;
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

vector<CheckTask::Ptr> CheckTask::GetFinishedTasks(void)
{
	mutex::scoped_lock lock(m_FinishedTasksMutex);

	vector<CheckTask::Ptr> result = m_FinishedTasks;
	m_FinishedTasks.clear();

	return result;
}

void CheckTask::FinishTask(const CheckTask::Ptr& task)
{
	mutex::scoped_lock lock(m_FinishedTasksMutex);
	m_FinishedTasks.push_back(task);
}
