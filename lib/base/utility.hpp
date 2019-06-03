/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef UTILITY_H
#define UTILITY_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "base/array.hpp"
#include "base/threadpool.hpp"
#include <boost/thread/tss.hpp>
#include <typeinfo>
#include <vector>

namespace icinga
{

#ifdef _WIN32
#define MS_VC_EXCEPTION 0x406D1388

#	pragma pack(push, 8)
struct THREADNAME_INFO
{
	DWORD dwType;
	LPCSTR szName;
	DWORD dwThreadID;
	DWORD dwFlags;
};
#	pragma pack(pop)
#endif

enum GlobType
{
	GlobFile = 1,
	GlobDirectory = 2
};

/**
 * Helper functions.
 *
 * @ingroup base
 */
class Utility
{
public:
	static String DemangleSymbolName(const String& sym);
	static String GetTypeName(const std::type_info& ti);
	static String GetSymbolName(const void *addr);

	static bool Match(const String& pattern, const String& text);
	static bool CidrMatch(const String& pattern, const String& ip);

	static String DirName(const String& path);
	static String BaseName(const String& path);

	static String GetEnv(const String& key);

	static void NullDeleter(void *);

	static double GetTime();

	static pid_t GetPid();

	static void Sleep(double timeout);

	static String NewUniqueID();

	static bool Glob(const String& pathSpec, const std::function<void (const String&)>& callback, int type = GlobFile | GlobDirectory);
	static bool GlobRecursive(const String& path, const String& pattern, const std::function<void (const String&)>& callback, int type = GlobFile | GlobDirectory);
	static void MkDir(const String& path, int mode);
	static void MkDirP(const String& path, int mode);
	static bool SetFileOwnership(const String& file, const String& user, const String& group);

	static void QueueAsyncCallback(const std::function<void ()>& callback, SchedulerPolicy policy = DefaultScheduler);

	static String NaturalJoin(const std::vector<String>& tokens);
	static String Join(const Array::Ptr& tokens, char separator, bool escapeSeparator = true);

	static String FormatDuration(double duration);
	static String FormatDateTime(const char *format, double ts);
	static String FormatErrorNumber(int code);

#ifndef _WIN32
	static void SetNonBlocking(int fd, bool nb = true);
	static void SetCloExec(int fd, bool cloexec = true);
#endif /* _WIN32 */

	static void SetNonBlockingSocket(SOCKET s, bool nb = true);

	static String EscapeShellCmd(const String& s);
	static String EscapeShellArg(const String& s);
#ifdef _WIN32
	static String EscapeCreateProcessArg(const String& arg);
#endif /* _WIN32 */

	static String EscapeString(const String& s, const String& chars, const bool illegal);
	static String UnescapeString(const String& s);

	static void SetThreadName(const String& name, bool os = true);
	static String GetThreadName();

	static unsigned long SDBM(const String& str, size_t len = String::NPos);

	static int CompareVersion(const String& v1, const String& v2);

	static int Random();

	static String GetHostName();
	static String GetFQDN();

	static tm LocalTime(time_t ts);

	static bool PathExists(const String& path);

	static void Remove(const String& path);
	static void RemoveDirRecursive(const String& path);
	static void CopyFile(const String& source, const String& target);
	static void RenameFile(const String& source, const String& target);

	static Value LoadJsonFile(const String& path);
	static void SaveJsonFile(const String& path, int mode, const Value& value);

	static String GetPlatformKernel();
	static String GetPlatformKernelVersion();
	static String GetPlatformName();
	static String GetPlatformVersion();
	static String GetPlatformArchitecture();

	static String ValidateUTF8(const String& input);

	static String CreateTempFile(const String& path, int mode, std::fstream& fp);

#ifdef _WIN32
	static String GetIcingaInstallPath();
	static String GetIcingaDataPath();
#endif /* _WIN32 */

	static String GetFromEnvironment(const String& env);

	static bool ComparePasswords(const String& enteredPassword, const String& actualPassword);

#ifdef I2_DEBUG
	static void SetTime(double);
	static void IncrementTime(double);
#endif /* I2_DEBUG */

private:
	Utility();

#ifdef _WIN32
	static int MksTemp (char *tmpl);
#endif /* _WIN32 */

#ifdef I2_DEBUG
	static double m_DebugTime;
#endif /* I2_DEBUG */

	static boost::thread_specific_ptr<String> m_ThreadName;
	static boost::thread_specific_ptr<unsigned int> m_RandSeed;
};

}

#endif /* UTILITY_H */
