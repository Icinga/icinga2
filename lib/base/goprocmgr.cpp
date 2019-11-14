/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#include "base/goprocmgr.hpp"
#include <boost/utility/string_view.hpp>
#include <cstdint>
#include <istream>
#include <ostream>
#include <sys/types.h>

using namespace icinga;

struct RawBytesArray32
{
	uint32_t Length;
};

struct RawSpawnRequest
{
	uint64_t IPid;
	uint32_t Args;
	uint32_t ExtraEnv;
	double Timeout;
};

std::ostream& operator<<(std::ostream& os, const SpawnRequest& sr)
{
	{
		RawSpawnRequest rsr {sr.IPid, (uint32_t)sr.Args.size(), (uint32_t)sr.ExtraEnv.size(), sr.Timeout};
		os << boost::string_view((const char*)(void*)&rsr, sizeof(rsr));
	}

	for (auto arr : {&sr.Args, &sr.ExtraEnv}) {
		for (auto& str : *arr) {
			RawBytesArray32 rba {(uint32_t)str.GetLength()};
			os << boost::string_view((const char*)(void*)&rba, sizeof(rba));
		}
	}

	for (auto arr : {&sr.Args, &sr.ExtraEnv}) {
		for (auto& str : *arr) {
			os << str;
		}
	}

	return os;
}

struct RawExitStatus
{
	uint64_t IPid;
	int32_t Pid;
	int32_t ExitCode;
	uint64_t Output;
	double ExecStart;
	double ExecEnd;
};

std::istream& operator>>(std::istream& is, ExitStatus& es)
{
	{
		RawExitStatus res;
		is.read((char*)(void*)&res, sizeof(res));

		es = ExitStatus{res.IPid, res.Pid, res.ExitCode, String(res.Output, 0), res.ExecStart, res.ExecEnd};
	}

	is.read((char*)(void*)&*es.Output.Begin(), es.Output.GetLength());

	return is;
}
