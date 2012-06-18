#include "i2-icinga.h"

using namespace icinga;

vector<ThreadPool::Task> NagiosCheckTask::m_QueuedTasks;

NagiosCheckTask::NagiosCheckTask(const Service& service)
	: CheckTask(service)
{
	string checkCommand = service.GetCheckCommand();
	m_Command = MacroProcessor::ResolveMacros(checkCommand, service.GetMacros()) + " 2>&1";

	m_Task = packaged_task<CheckResult>(boost::bind(&NagiosCheckTask::RunCheck, this));
	m_Result = m_Task.get_future();
}

void NagiosCheckTask::Enqueue(void)
{
	m_QueuedTasks.push_back(bind(&NagiosCheckTask::Execute, this));
}

void NagiosCheckTask::FlushQueue(void)
{
	ThreadPool::GetDefaultPool()->EnqueueTasks(m_QueuedTasks);
	m_QueuedTasks.clear();
}

bool NagiosCheckTask::IsFinished(void) const
{
	return m_Result.has_value();
}

CheckResult NagiosCheckTask::GetResult(void)
{
	return m_Result.get();
}

void NagiosCheckTask::Execute(void)
{
	m_Task();
}

CheckResult NagiosCheckTask::RunCheck(void) const
{
	CheckResult cr;
	FILE *fp;

	time(&cr.StartTime);

#ifdef _MSC_VER
	fp = _popen(m_Command.c_str(), "r");
#else /* _MSC_VER */
	fp = popen(m_Command.c_str(), "r");
#endif /* _MSC_VER */

	stringstream outputbuf;

	while (!feof(fp)) {
		char buffer[128];
		size_t read = fread(buffer, 1, sizeof(buffer), fp);

		if (read == 0)
			break;

		outputbuf << string(buffer, buffer + read);
	}

	cr.Output = outputbuf.str();
	boost::algorithm::trim(cr.Output);

	int status, exitcode;
#ifdef _MSC_VER
	status = _pclose(fp);
#else /* _MSC_VER */
	status = pclose(fp);
#endif /* _MSC_VER */

#ifndef _MSC_VER
	if (WIFEXITED(status)) {
		exitcode = WEXITSTATUS(status);
#else /* _MSC_VER */
		exitcode = status;
#endif /* _MSC_VER */

		switch (exitcode) {
			case 0:
				cr.State = StateOK;
				break;
			case 1:
				cr.State = StateWarning;
				break;
			case 2:
				cr.State = StateCritical;
				break;
			default:
				cr.State = StateUnknown;
				break;
		}
#ifndef _MSC_VER
	} else if (WIFSIGNALED(status)) {
		cr.Output = "Process was terminated by signal " + WTERMSIG(status);
		cr.State = StateUnknown;
	}
#endif /* _MSC_VER */

	time(&cr.EndTime);

	return cr;
}

CheckTask::Ptr NagiosCheckTask::CreateTask(const Service& service)
{
	assert(service.GetCheckType() == "nagios");

	return boost::make_shared<NagiosCheckTask>(service);
}
