/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef GOPROCMGR_H
#define GOPROCMGR_H

#include "base/string.hpp"
#include <cstdint>
#include <istream>
#include <ostream>
#include <sys/types.h>
#include <vector>

namespace icinga
{

struct SpawnRequest
{
	uint64_t IPid;
	std::vector<String> Args;
	std::vector<String> ExtraEnv;
	double Timeout;
};

struct ExitStatus
{
	uint64_t IPid;
	pid_t Pid;
	int ExitCode;
	String Output;
	double ExecStart;
	double ExecEnd;
};

}

std::ostream& operator<<(std::ostream&, const icinga::SpawnRequest&);
std::istream& operator>>(std::istream&, icinga::ExitStatus&);

#endif /* GOPROCMGR_H */
