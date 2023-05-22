/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "base/dictionary.hpp"
#include <iosfwd>
#include <deque>
#include <vector>
#include <sstream>
#include <mutex>
#include <condition_variable>

namespace icinga
{

/**
 * The result of a Process task.
 *
 * @ingroup base
 */
struct ProcessResult
{
	pid_t PID;
	double ExecutionStart;
	double ExecutionEnd;
	long ExitStatus;
	String Output;
};

/**
 * A process task. Executes an external application and returns the exit
 * code and console output.
 *
 * @ingroup base
 */
class Process final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Process);

#ifdef _WIN32
	typedef String Arguments;
	typedef HANDLE ProcessHandle;
	typedef HANDLE ConsoleHandle;
#else /* _WIN32 */
	typedef std::vector<String> Arguments;
	typedef pid_t ProcessHandle;
	typedef int ConsoleHandle;
#endif /* _WIN32 */

	static const std::deque<Process::Ptr>::size_type MaxTasksPerThread = 512;

	Process(Arguments arguments, Dictionary::Ptr extraEnvironment = nullptr);
	~Process() override;

	void SetTimeout(double timeout);
	double GetTimeout() const;

	void SetAdjustPriority(bool adjust);
	bool GetAdjustPriority() const;

	void Run(const std::function<void (const ProcessResult&)>& callback = std::function<void (const ProcessResult&)>());

	const ProcessResult& WaitForResult();

	pid_t GetPID() const;

	static Arguments PrepareCommand(const Value& command);

	static void ThreadInitialize();

	static String PrettyPrintArguments(const Arguments& arguments);

#ifndef _WIN32
	static void InitializeSpawnHelper();
#endif /* _WIN32 */

private:
	Arguments m_Arguments;
	Dictionary::Ptr m_ExtraEnvironment;

	double m_Timeout;
#ifndef _WIN32
	bool m_SentSigterm;
#endif /* _WIN32 */

	bool m_AdjustPriority;

	ProcessHandle m_Process;
	pid_t m_PID;
	ConsoleHandle m_FD;

#ifdef _WIN32
	bool m_ReadPending;
	bool m_ReadFailed;
	OVERLAPPED m_Overlapped;
	char m_ReadBuffer[1024];
#endif /* _WIN32 */

	std::ostringstream m_OutputStream;
	std::function<void (const ProcessResult&)> m_Callback;
	ProcessResult m_Result;
	bool m_ResultAvailable;
	std::mutex m_ResultMutex;
	std::condition_variable m_ResultCondition;

	static void IOThreadProc(int tid);
	bool DoEvents();
	int GetTID() const;
	double GetNextTimeout() const;
};

}
