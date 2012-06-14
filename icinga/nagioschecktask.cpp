#include "i2-icinga.h"

using namespace icinga;

NagiosCheckTask::NagiosCheckTask(const Service& service)
{
	string checkCommand = service.GetCheckCommand();
	m_Command = MacroProcessor::ResolveMacros(checkCommand, service.GetMacros());
}

CheckResult NagiosCheckTask::Execute(void) const
{
	CheckResult cr;
	FILE *fp;

	time(&cr.StartTime);

	string command = m_Command + " 2>&1";

#ifdef _MSC_VER
	fp = _popen(command.c_str(), "r");
#else /* _MSC_VER */
	fp = popen(command.c_str(), "r");
#endif /* _MSC_VER */

	stringstream output;

	while (!feof(fp)) {
		char buffer[128];
		size_t read = fread(buffer, 1, sizeof(buffer), fp);

		if (read == 0)
			break;

		output << string(buffer, buffer + read);
	}

	cr.Output = output.str();

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

	return make_shared<NagiosCheckTask>(service);
}
