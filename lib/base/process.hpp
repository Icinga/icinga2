/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef PROCESS_H
#define PROCESS_H

#include "base/i2-base.hpp"
#include "base/dictionary.hpp"
#include <iosfwd>
#include <deque>
#include <vector>
#include <sstream>

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

	typedef std::function<void (const ProcessResult&)> Callback;

#ifdef _WIN32
	typedef String Arguments;
	typedef HANDLE ProcessHandle;
	typedef HANDLE ConsoleHandle;

	static const std::deque<Process::Ptr>::size_type MaxTasksPerThread = 512;
#else /* _WIN32 */
	typedef std::vector<String> Arguments;
#endif /* _WIN32 */

	Process(Arguments arguments, Dictionary::Ptr extraEnvironment = nullptr);
	~Process() override;

	void SetTimeout(double timeout);
	double GetTimeout() const;

	void Run(Callback callback = Callback());

	static Arguments PrepareCommand(const Value& command);

	static String PrettyPrintArguments(const Arguments& arguments);

#ifdef _WIN32
	pid_t GetPID() const;

	static void ThreadInitialize();
#else /* _WIN32 */
	static void InitializeSpawnHelper();
#endif /* _WIN32 */

private:
	Arguments m_Arguments;
	Dictionary::Ptr m_ExtraEnvironment;

	double m_Timeout;

#ifdef _WIN32
	ProcessHandle m_Process;
	pid_t m_PID;
	ConsoleHandle m_FD;

	bool m_ReadPending;
	bool m_ReadFailed;
	OVERLAPPED m_Overlapped;
	char m_ReadBuffer[1024];

	std::ostringstream m_OutputStream;
	Callback m_Callback;
	ProcessResult m_Result;

	static void IOThreadProc(int tid);
	bool DoEvents();
	int GetTID() const;
#endif /* _WIN32 */
};

}

#endif /* PROCESS_H */
