/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef GOPROCMGR_H
#define GOPROCMGR_H

#include "base/string.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>
#include <cstdint>
#include <sys/types.h>
#include <vector>

namespace icinga
{

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

struct SpawnRequest
{
	uint64_t IPid;
	std::vector<String> Args;
	std::vector<String> ExtraEnv;
	double Timeout;

	template<class AsyncWriteStream>
	inline
	void WriteTo(AsyncWriteStream& aws, boost::asio::yield_context yc) const
	{
		namespace asio = boost::asio;

		{
			RawSpawnRequest rsr {IPid, (uint32_t)Args.size(), (uint32_t)ExtraEnv.size(), Timeout};
			asio::async_write(aws, asio::const_buffer(&rsr, sizeof(rsr)), yc);
		}

		for (auto arr : {&Args, &ExtraEnv}) {
			for (auto& str : *arr) {
				RawBytesArray32 rba {(uint32_t)str.GetLength()};
				asio::async_write(aws, asio::const_buffer(&rba, sizeof(rba)), yc);
			}
		}

		for (auto arr : {&Args, &ExtraEnv}) {
			for (auto& str : *arr) {
				asio::async_write(aws, asio::const_buffer(str.CStr(), str.GetLength()), yc);
			}
		}
	}
};

struct RawExitStatus
{
	uint64_t IPid;
	int32_t Pid;
	int32_t ExitCode;
	uint64_t Output;
	double ExecStart;
	double ExecEnd;
};

struct ExitStatus
{
	uint64_t IPid;
	pid_t Pid;
	int ExitCode;
	String Output;
	double ExecStart;
	double ExecEnd;

	template<class AsyncReadStream>
	inline
	void ReadFrom(AsyncReadStream& ars, boost::asio::yield_context yc)
	{
		namespace asio = boost::asio;

		{
			RawExitStatus res;
			asio::async_read(ars, asio::mutable_buffer(&res, sizeof(res)), yc);

			*this = ExitStatus{res.IPid, res.Pid, res.ExitCode, String(res.Output, 0), res.ExecStart, res.ExecEnd};
		}

		asio::async_read(ars, asio::mutable_buffer(&*Output.GetData().begin(), Output.GetLength()), yc);
	}
};

}

#endif /* GOPROCMGR_H */
