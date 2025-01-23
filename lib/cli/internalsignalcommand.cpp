/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/internalsignalcommand.hpp"
#include "base/logger.hpp"
#include <signal.h>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("internal/signal", InternalSignalCommand);

String InternalSignalCommand::GetDescription() const
{
	return "Send signal as Icinga user";
}

String InternalSignalCommand::GetShortDescription() const
{
	return "Send signal as Icinga user";
}

bool InternalSignalCommand::IsHidden() const
{
	return true;
}

void InternalSignalCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("pid,p", po::value<int>(), "Target PID")
		("sig,s", po::value<String>(), "Signal (POSIX string) to send")
	;
}

/**
 * The entry point for the "internal signal" CLI command.
 *
 * @returns An exit status.
 */
int InternalSignalCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
#ifndef _WIN32
	String signal = vm["sig"].as<String>();

	/* Thank POSIX */
	if (signal == "SIGKILL")
		return kill(vm["pid"].as<int>(), SIGKILL);
	if (signal == "SIGINT")
		return kill(vm["pid"].as<int>(), SIGINT);
	if (signal == "SIGCHLD")
		return kill(vm["pid"].as<int>(), SIGCHLD);
	if (signal == "SIGHUP")
		return kill(vm["pid"].as<int>(), SIGHUP);

	Log(LogCritical, "cli") << "Unsupported signal \"" << signal << "\"";
#else
	Log(LogCritical, "cli", "Unsupported action on Windows.");
#endif /* _Win32 */
	return 1;
}
