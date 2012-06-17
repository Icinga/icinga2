#include "i2-icinga.h"

using namespace icinga;

NagiosCheckTask::NagiosCheckTask(const Service& service)
{
	string checkCommand = service.GetCheckCommand();
	m_Command = MacroProcessor::ResolveMacros(checkCommand, service.GetMacros()) + " 2>&1";

	m_Task = packaged_task<CheckResult>(boost::bind(&NagiosCheckTask::RunCheck, this));
	m_Result = m_Task.get_future();
}

void NagiosCheckTask::Execute(void)
{
	Application::Log(LogDebug, "icinga", "Nagios check command: " + m_Command);

	ThreadPool::GetDefaultPool()->EnqueueTask(boost::bind(&NagiosCheckTask::InternalExecute, this));
}

void NagiosCheckTask::InternalExecute(void)
{
	m_Task();
}

bool NagiosCheckTask::IsFinished(void) const
{
	return m_Result.is_ready();
}

CheckResult NagiosCheckTask::GetResult(void)
{
	return m_Result.get();
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
