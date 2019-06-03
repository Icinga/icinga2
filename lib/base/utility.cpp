/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/application.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/socket.hpp"
#include "base/utility.hpp"
#include "base/json.hpp"
#include "base/objectlock.hpp"
#include <cstdint>
#include <mmatch.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/tss.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <ios>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdlib.h>
#include <future>
#include <utf8.h>
#include <vector>

#ifdef __FreeBSD__
#	include <pthread_np.h>
#endif /* __FreeBSD__ */

#ifdef HAVE_CXXABI_H
#	include <cxxabi.h>
#endif /* HAVE_CXXABI_H */

#ifndef _WIN32
#	include <sys/types.h>
#	include <sys/utsname.h>
#	include <pwd.h>
#	include <grp.h>
#	include <errno.h>
#endif /* _WIN32 */

#ifdef _WIN32
#	include <VersionHelpers.h>
#	include <windows.h>
#	include <io.h>
#	include <msi.h>
#	include <shlobj.h>
#endif /*_WIN32*/

using namespace icinga;

boost::thread_specific_ptr<String> Utility::m_ThreadName;
boost::thread_specific_ptr<unsigned int> Utility::m_RandSeed;

#ifdef I2_DEBUG
double Utility::m_DebugTime = -1;
#endif /* I2_DEBUG */

/**
 * Demangles a symbol name.
 *
 * @param sym The symbol name.
 * @returns A human-readable version of the symbol name.
 */
String Utility::DemangleSymbolName(const String& sym)
{
	String result = sym;

#ifdef HAVE_CXXABI_H
	int status;
	char *realname = abi::__cxa_demangle(sym.CStr(), nullptr, nullptr, &status);

	if (realname) {
		result = String(realname);
		free(realname);
	}
#elif defined(_MSC_VER) /* HAVE_CXXABI_H */
	CHAR output[256];

	if (UnDecorateSymbolName(sym.CStr(), output, sizeof(output), UNDNAME_COMPLETE) > 0)
		result = output;
#else /* _MSC_VER */
	/* We're pretty much out of options here. */
#endif /* _MSC_VER */

	return result;
}

/**
 * Returns a human-readable type name of a type_info object.
 *
 * @param ti A type_info object.
 * @returns The type name of the object.
 */
String Utility::GetTypeName(const std::type_info& ti)
{
	return DemangleSymbolName(ti.name());
}

String Utility::GetSymbolName(const void *addr)
{
#ifdef HAVE_DLADDR
	Dl_info dli;

	if (dladdr(const_cast<void *>(addr), &dli) > 0)
		return dli.dli_sname;
#endif /* HAVE_DLADDR */

#ifdef _WIN32
	char buffer[sizeof(SYMBOL_INFO)+MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;

	DWORD64 dwAddress = (DWORD64)addr;
	DWORD64 dwDisplacement;

	IMAGEHLP_LINE64 line;
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	if (SymFromAddr(GetCurrentProcess(), dwAddress, &dwDisplacement, pSymbol)) {
		char output[256];
		if (UnDecorateSymbolName(pSymbol->Name, output, sizeof(output), UNDNAME_COMPLETE))
			return String(output) + "+" + Convert::ToString(dwDisplacement);
		else
			return String(pSymbol->Name) + "+" + Convert::ToString(dwDisplacement);
	}
#endif /* _WIN32 */

	return "(unknown function)";
}

/**
 * Performs wildcard pattern matching.
 *
 * @param pattern The wildcard pattern.
 * @param text The String that should be checked.
 * @returns true if the wildcard pattern matches, false otherwise.
 */
bool Utility::Match(const String& pattern, const String& text)
{
	return (match(pattern.CStr(), text.CStr()) == 0);
}

static bool ParseIp(const String& ip, char addr[16], int *proto)
{
	if (inet_pton(AF_INET, ip.CStr(), addr + 12) == 1) {
		/* IPv4-mapped IPv6 address (::ffff:<ipv4-bits>) */
		memset(addr, 0, 10);
		memset(addr + 10, 0xff, 2);
		*proto = AF_INET;

		return true;
	}

	if (inet_pton(AF_INET6, ip.CStr(), addr) == 1) {
		*proto = AF_INET6;

		return true;
	}

	return false;
}

static void ParseIpMask(const String& ip, char mask[16], int *bits)
{
	String::SizeType slashp = ip.FindFirstOf("/");
	String uip;

	if (slashp == String::NPos) {
		uip = ip;
		*bits = 0;
	} else {
		uip = ip.SubStr(0, slashp);
		*bits = Convert::ToLong(ip.SubStr(slashp + 1));
	}

	int proto;

	if (!ParseIp(uip, mask, &proto))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid IP address specified."));

	if (proto == AF_INET) {
		if (*bits > 32 || *bits < 0)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Mask must be between 0 and 32 for IPv4 CIDR masks."));

		*bits += 96;
	}

	if (slashp == String::NPos)
		*bits = 128;

	if (*bits > 128 || *bits < 0)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Mask must be between 0 and 128 for IPv6 CIDR masks."));

	for (int i = 0; i < 16; i++) {
		int lbits = std::max(0, *bits - i * 8);

		if (lbits >= 8)
			continue;

		if (mask[i] & (0xff >> lbits))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Masked-off bits must all be zero."));
	}
}

static bool IpMaskCheck(char addr[16], char mask[16], int bits)
{
	for (int i = 0; i < 16; i++) {
		if (bits < 8)
			return !((addr[i] ^ mask[i]) >> (8 - bits));

		if (mask[i] != addr[i])
			return false;

		bits -= 8;

		if (bits == 0)
			return true;
	}

	return true;
}

bool Utility::CidrMatch(const String& pattern, const String& ip)
{
	char mask[16];
	int bits;

	ParseIpMask(pattern, mask, &bits);

	char addr[16];
	int proto;

	if (!ParseIp(ip, addr, &proto))
		return false;

	return IpMaskCheck(addr, mask, bits);
}

/**
 * Returns the directory component of a path. See dirname(3) for details.
 *
 * @param path The full path.
 * @returns The directory.
 */
String Utility::DirName(const String& path)
{
	return boost::filesystem::path(path.Begin(), path.End()).parent_path().string();
}

/**
 * Returns the file component of a path. See basename(3) for details.
 *
 * @param path The full path.
 * @returns The filename.
 */
String Utility::BaseName(const String& path)
{
	return boost::filesystem::path(path.Begin(), path.End()).filename().string();
}

/**
 * Null deleter. Used as a parameter for the shared_ptr constructor.
 *
 * @param - The object that should be deleted.
 */
void Utility::NullDeleter(void *)
{
	/* Nothing to do here. */
}

#ifdef I2_DEBUG
/**
 * (DEBUG / TESTING ONLY) Sets the current system time to a static value,
 * that will be be retrieved by any component of Icinga, when using GetTime().
 *
 * This should be only used for testing purposes, e.g. unit tests and debugging of certain functionalities.
 */
void Utility::SetTime(double time)
{
	m_DebugTime = time;
}

/**
 * (DEBUG / TESTING ONLY) Increases the set debug system time by X seconds.
 *
 * This should be only used for testing purposes, e.g. unit tests and debugging of certain functionalities.
 */
void Utility::IncrementTime(double diff)
{
	m_DebugTime += diff;
}
#endif /* I2_DEBUG */

/**
 * Returns the current UNIX timestamp including fractions of seconds.
 *
 * @returns The current time.
 */
double Utility::GetTime()
{
#ifdef I2_DEBUG
	if (m_DebugTime >= 0) {
		// (DEBUG / TESTING ONLY) this will return a *STATIC* system time, if the value has been set!
		return m_DebugTime;
	}
#endif /* I2_DEBUG */
#ifdef _WIN32
	FILETIME cft;
	GetSystemTimeAsFileTime(&cft);

	ULARGE_INTEGER ucft;
	ucft.HighPart = cft.dwHighDateTime;
	ucft.LowPart = cft.dwLowDateTime;

	SYSTEMTIME est = { 1970, 1, 4, 1, 0, 0, 0, 0};
	FILETIME eft;
	SystemTimeToFileTime(&est, &eft);

	ULARGE_INTEGER ueft;
	ueft.HighPart = eft.dwHighDateTime;
	ueft.LowPart = eft.dwLowDateTime;

	return ((ucft.QuadPart - ueft.QuadPart) / 10000) / 1000.0;
#else /* _WIN32 */
	struct timeval tv;

	int rc = gettimeofday(&tv, nullptr);
	VERIFY(rc >= 0);

	return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif /* _WIN32 */
}

/**
 * Returns the ID of the current process.
 *
 * @returns The PID.
 */
pid_t Utility::GetPid()
{
#ifndef _WIN32
	return getpid();
#else /* _WIN32 */
	return GetCurrentProcessId();
#endif /* _WIN32 */
}

/**
 * Sleeps for the specified amount of time.
 *
 * @param timeout The timeout in seconds.
 */
void Utility::Sleep(double timeout)
{
#ifndef _WIN32
	unsigned long micros = timeout * 1000000u;
	if (timeout >= 1.0)
		sleep((unsigned)timeout);

	usleep(micros % 1000000u);
#else /* _WIN32 */
	::Sleep(timeout * 1000);
#endif /* _WIN32 */
}

/**
 * Generates a new unique ID.
 *
 * @returns The new unique ID.
 */
String Utility::NewUniqueID()
{
	return boost::lexical_cast<std::string>(boost::uuids::random_generator()());
}

#ifdef _WIN32
static bool GlobHelper(const String& pathSpec, int type, std::vector<String>& files, std::vector<String>& dirs)
{
	HANDLE handle;
	WIN32_FIND_DATA wfd;

	handle = FindFirstFile(pathSpec.CStr(), &wfd);

	if (handle == INVALID_HANDLE_VALUE) {
		DWORD errorCode = GetLastError();

		if (errorCode == ERROR_FILE_NOT_FOUND)
			return false;

		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("FindFirstFile")
			<< errinfo_win32_error(errorCode)
			<< boost::errinfo_file_name(pathSpec));
	}

	do {
		if (strcmp(wfd.cFileName, ".") == 0 || strcmp(wfd.cFileName, "..") == 0)
			continue;

		String path = Utility::DirName(pathSpec) + "/" + wfd.cFileName;

		if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (type & GlobDirectory))
			dirs.push_back(path);
		else if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (type & GlobFile))
			files.push_back(path);
	} while (FindNextFile(handle, &wfd));

	if (!FindClose(handle)) {
		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("FindClose")
			<< errinfo_win32_error(GetLastError()));
	}

	return true;
}
#endif /* _WIN32 */

#ifndef _WIN32
static int GlobErrorHandler(const char *epath, int eerrno)
{
	if (eerrno == ENOTDIR)
		return 0;

	return eerrno;
}
#endif /* _WIN32 */

/**
 * Calls the specified callback for each file matching the path specification.
 *
 * @param pathSpec The path specification.
 * @param callback The callback which is invoked for each matching file.
 * @param type The file type (a combination of GlobFile and GlobDirectory)
 */
bool Utility::Glob(const String& pathSpec, const std::function<void (const String&)>& callback, int type)
{
	std::vector<String> files, dirs;

#ifdef _WIN32
	std::vector<String> tokens = pathSpec.Split("\\/");

	String part1;

	for (std::vector<String>::size_type i = 0; i < tokens.size() - 1; i++) {
		const String& token = tokens[i];

		if (!part1.IsEmpty())
			part1 += "/";

		part1 += token;

		if (token.FindFirstOf("?*") != String::NPos) {
			String part2;

			for (std::vector<String>::size_type k = i + 1; k < tokens.size(); k++) {
				if (!part2.IsEmpty())
					part2 += "/";

				part2 += tokens[k];
			}

			std::vector<String> files2, dirs2;

			if (!GlobHelper(part1, GlobDirectory, files2, dirs2))
				return false;

			for (const String& dir : dirs2) {
				if (!Utility::Glob(dir + "/" + part2, callback, type))
					return false;
			}

			return true;
		}
	}

	if (!GlobHelper(part1 + "/" + tokens[tokens.size() - 1], type, files, dirs))
		return false;
#else /* _WIN32 */
	glob_t gr;

	int rc = glob(pathSpec.CStr(), GLOB_NOSORT, GlobErrorHandler, &gr);

	if (rc) {
		if (rc == GLOB_NOMATCH)
			return false;

		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("glob")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(pathSpec));
	}

	if (gr.gl_pathc == 0) {
		globfree(&gr);
		return false;
	}

	size_t left;
	char **gp;
	for (gp = gr.gl_pathv, left = gr.gl_pathc; left > 0; gp++, left--) {
		struct stat statbuf;

		if (stat(*gp, &statbuf) < 0)
			continue;

		if (!S_ISDIR(statbuf.st_mode) && !S_ISREG(statbuf.st_mode))
			continue;

		if (S_ISDIR(statbuf.st_mode) && (type & GlobDirectory))
			dirs.emplace_back(*gp);
		else if (!S_ISDIR(statbuf.st_mode) && (type & GlobFile))
			files.emplace_back(*gp);
	}

	globfree(&gr);
#endif /* _WIN32 */

	std::sort(files.begin(), files.end());
	for (const String& cpath : files) {
		callback(cpath);
	}

	std::sort(dirs.begin(), dirs.end());
	for (const String& cpath : dirs) {
		callback(cpath);
	}

	return true;
}

/**
 * Calls the specified callback for each file in the specified directory
 * or any of its child directories if the file name matches the specified
 * pattern.
 *
 * @param path The path.
 * @param pattern The pattern.
 * @param callback The callback which is invoked for each matching file.
 * @param type The file type (a combination of GlobFile and GlobDirectory)
 */
bool Utility::GlobRecursive(const String& path, const String& pattern, const std::function<void (const String&)>& callback, int type)
{
	std::vector<String> files, dirs, alldirs;

#ifdef _WIN32
	HANDLE handle;
	WIN32_FIND_DATA wfd;

	String pathSpec = path + "/*";

	handle = FindFirstFile(pathSpec.CStr(), &wfd);

	if (handle == INVALID_HANDLE_VALUE) {
		DWORD errorCode = GetLastError();

		if (errorCode == ERROR_FILE_NOT_FOUND)
			return false;

		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("FindFirstFile")
			<< errinfo_win32_error(errorCode)
			<< boost::errinfo_file_name(pathSpec));
	}

	do {
		if (strcmp(wfd.cFileName, ".") == 0 || strcmp(wfd.cFileName, "..") == 0)
			continue;

		String cpath = path + "/" + wfd.cFileName;

		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			alldirs.push_back(cpath);

		if (!Utility::Match(pattern, wfd.cFileName))
			continue;

		if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (type & GlobFile))
			files.push_back(cpath);

		if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (type & GlobDirectory))
			dirs.push_back(cpath);
	} while (FindNextFile(handle, &wfd));

	if (!FindClose(handle)) {
		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("FindClose")
			<< errinfo_win32_error(GetLastError()));
	}
#else /* _WIN32 */
	DIR *dirp;

	dirp = opendir(path.CStr());

	if (!dirp)
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("opendir")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(path));

	while (dirp) {
		dirent *pent;

		errno = 0;
		pent = readdir(dirp);
		if (!pent && errno != 0) {
			closedir(dirp);

			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("readdir")
				<< boost::errinfo_errno(errno)
				<< boost::errinfo_file_name(path));
		}

		if (!pent)
			break;

		if (strcmp(pent->d_name, ".") == 0 || strcmp(pent->d_name, "..") == 0)
			continue;

		String cpath = path + "/" + pent->d_name;

		struct stat statbuf;

		if (stat(cpath.CStr(), &statbuf) < 0)
			continue;

		if (S_ISDIR(statbuf.st_mode))
			alldirs.push_back(cpath);

		if (!Utility::Match(pattern, pent->d_name))
			continue;

		if (S_ISDIR(statbuf.st_mode) && (type & GlobDirectory))
			dirs.push_back(cpath);

		if (!S_ISDIR(statbuf.st_mode) && (type & GlobFile))
			files.push_back(cpath);
	}

	closedir(dirp);

#endif /* _WIN32 */

	std::sort(files.begin(), files.end());
	for (const String& cpath : files) {
		callback(cpath);
	}

	std::sort(dirs.begin(), dirs.end());
	for (const String& cpath : dirs) {
		callback(cpath);
	}

	std::sort(alldirs.begin(), alldirs.end());
	for (const String& cpath : alldirs) {
		GlobRecursive(cpath, pattern, callback, type);
	}

	return true;
}


void Utility::MkDir(const String& path, int mode)
{

#ifndef _WIN32
	if (mkdir(path.CStr(), mode) < 0 && errno != EEXIST) {
#else /*_ WIN32 */
	if (mkdir(path.CStr()) < 0 && errno != EEXIST) {
#endif /* _WIN32 */

		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("mkdir")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(path));
	}
}

void Utility::MkDirP(const String& path, int mode)
{
	size_t pos = 0;

	while (pos != String::NPos) {
#ifndef _WIN32
		pos = path.Find("/", pos + 1);
#else /*_ WIN32 */
		pos = path.FindFirstOf("/\\", pos + 1);
#endif /* _WIN32 */

		String spath = path.SubStr(0, pos + 1);
		struct stat statbuf;
		if (stat(spath.CStr(), &statbuf) < 0 && errno == ENOENT)
			MkDir(path.SubStr(0, pos), mode);
	}
}

void Utility::Remove(const String& path)
{
	namespace fs = boost::filesystem;

	(void)fs::remove(fs::path(path.Begin(), path.End()));
}

void Utility::RemoveDirRecursive(const String& path)
{
	namespace fs = boost::filesystem;

	(void)fs::remove_all(fs::path(path.Begin(), path.End()));
}

/*
 * Copies a source file to a target location.
 * Caller must ensure that the target's base directory exists and is writable.
 */
void Utility::CopyFile(const String& source, const String& target)
{
	namespace fs = boost::filesystem;

	fs::copy_file(fs::path(source.Begin(), source.End()), fs::path(target.Begin(), target.End()), fs::copy_option::overwrite_if_exists);
}

/*
 * Renames a source file to a target location.
 * Caller must ensure that the target's base directory exists and is writable.
 */
void Utility::RenameFile(const String& source, const String& target)
{
	namespace fs = boost::filesystem;

	fs::rename(fs::path(source.Begin(), source.End()), fs::path(target.Begin(), target.End()));
}

/*
 * Set file permissions
 */
bool Utility::SetFileOwnership(const String& file, const String& user, const String& group)
{
#ifndef _WIN32
	errno = 0;
	struct passwd *pw = getpwnam(user.CStr());

	if (!pw) {
		if (errno == 0) {
			Log(LogCritical, "cli")
				<< "Invalid user specified: " << user;
			return false;
		} else {
			Log(LogCritical, "cli")
				<< "getpwnam() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			return false;
		}
	}

	errno = 0;
	struct group *gr = getgrnam(group.CStr());

	if (!gr) {
		if (errno == 0) {
			Log(LogCritical, "cli")
				<< "Invalid group specified: " << group;
			return false;
		} else {
			Log(LogCritical, "cli")
				<< "getgrnam() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			return false;
		}
	}

	if (chown(file.CStr(), pw->pw_uid, gr->gr_gid) < 0) {
		Log(LogCritical, "cli")
			<< "chown() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
		return false;
	}
#endif /* _WIN32 */

	return true;
}

#ifndef _WIN32
void Utility::SetNonBlocking(int fd, bool nb)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("fcntl")
			<< boost::errinfo_errno(errno));
	}

	if (nb)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	if (fcntl(fd, F_SETFL, flags) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("fcntl")
			<< boost::errinfo_errno(errno));
	}
}

void Utility::SetCloExec(int fd, bool cloexec)
{
	int flags = fcntl(fd, F_GETFD, 0);

	if (flags < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("fcntl")
			<< boost::errinfo_errno(errno));
	}

	if (cloexec)
		flags |= FD_CLOEXEC;
	else
		flags &= ~FD_CLOEXEC;

	if (fcntl(fd, F_SETFD, flags) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("fcntl")
			<< boost::errinfo_errno(errno));
	}
}
#endif /* _WIN32 */

void Utility::SetNonBlockingSocket(SOCKET s, bool nb)
{
#ifndef _WIN32
	SetNonBlocking(s, nb);
#else /* _WIN32 */
	unsigned long lflag = nb;
	ioctlsocket(s, FIONBIO, &lflag);
#endif /* _WIN32 */
}

void Utility::QueueAsyncCallback(const std::function<void ()>& callback, SchedulerPolicy policy)
{
	Application::GetTP().Post(callback, policy);
}

String Utility::NaturalJoin(const std::vector<String>& tokens)
{
	String result;

	for (std::vector<String>::size_type i = 0; i < tokens.size(); i++) {
		result += tokens[i];

		if (tokens.size() > i + 1) {
			if (i < tokens.size() - 2)
				result += ", ";
			else if (i == tokens.size() - 2)
				result += " and ";
		}
	}

	return result;
}

String Utility::Join(const Array::Ptr& tokens, char separator, bool escapeSeparator)
{
	String result;
	bool first = true;

	ObjectLock olock(tokens);
	for (const Value& vtoken : tokens) {
		String token = Convert::ToString(vtoken);

		if (escapeSeparator) {
			boost::algorithm::replace_all(token, "\\", "\\\\");

			char sep_before[2], sep_after[3];
			sep_before[0] = separator;
			sep_before[1] = '\0';
			sep_after[0] = '\\';
			sep_after[1] = separator;
			sep_after[2] = '\0';
			boost::algorithm::replace_all(token, sep_before, sep_after);
		}

		if (first)
			first = false;
		else
			result += String(1, separator);

		result += token;
	}

	return result;
}

String Utility::FormatDuration(double duration)
{
	std::vector<String> tokens;
	String result;

	if (duration >= 86400) {
		int days = duration / 86400;
		tokens.emplace_back(Convert::ToString(days) + (days != 1 ? " days" : " day"));
		duration = static_cast<int>(duration) % 86400;
	}

	if (duration >= 3600) {
		int hours = duration / 3600;
		tokens.emplace_back(Convert::ToString(hours) + (hours != 1 ? " hours" : " hour"));
		duration = static_cast<int>(duration) % 3600;
	}

	if (duration >= 60) {
		int minutes = duration / 60;
		tokens.emplace_back(Convert::ToString(minutes) + (minutes != 1 ? " minutes" : " minute"));
		duration = static_cast<int>(duration) % 60;
	}

	if (duration >= 1) {
		int seconds = duration;
		tokens.emplace_back(Convert::ToString(seconds) + (seconds != 1 ? " seconds" : " second"));
	}

	if (tokens.size() == 0) {
		int milliseconds = std::floor(duration * 1000);
		if (milliseconds >= 1)
			tokens.emplace_back(Convert::ToString(milliseconds) + (milliseconds != 1 ? " milliseconds" : " millisecond"));
		else
			tokens.emplace_back("less than 1 millisecond");
	}

	return NaturalJoin(tokens);
}

String Utility::FormatDateTime(const char *format, double ts)
{
	char timestamp[128];
	auto tempts = (time_t)ts; /* We don't handle sub-second timestamps here just yet. */
	tm tmthen;

#ifdef _MSC_VER
	tm *temp = localtime(&tempts);

	if (!temp) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("localtime")
			<< boost::errinfo_errno(errno));
	}

	tmthen = *temp;
#else /* _MSC_VER */
	if (!localtime_r(&tempts, &tmthen)) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("localtime_r")
			<< boost::errinfo_errno(errno));
	}
#endif /* _MSC_VER */

	strftime(timestamp, sizeof(timestamp), format, &tmthen);

	return timestamp;
}

String Utility::FormatErrorNumber(int code) {
	std::ostringstream msgbuf;

#ifdef _WIN32
	char *message;
	String result = "Unknown error.";

	DWORD rc = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, (char *)&message,
		0, nullptr);

	if (rc != 0) {
		result = String(message);
		LocalFree(message);

		/* remove trailing new-line characters */
		boost::algorithm::trim_right(result);
	}

	msgbuf << code << ", \"" << result << "\"";
#else
	msgbuf << strerror(code);
#endif
	return msgbuf.str();
}

String Utility::EscapeShellCmd(const String& s)
{
	String result;
	size_t prev_quote = String::NPos;
	int index = -1;

	for (char ch : s) {
		bool escape = false;

		index++;

#ifdef _WIN32
		if (ch == '%' || ch == '"' || ch == '\'')
			escape = true;
#else /* _WIN32 */
		if (ch == '"' || ch == '\'') {
			/* Find a matching closing quotation character. */
			if (prev_quote == String::NPos && (prev_quote = s.FindFirstOf(ch, index + 1)) != String::NPos)
				; /* Empty statement. */
			else if (prev_quote != String::NPos && s[prev_quote] == ch)
				prev_quote = String::NPos;
			else
				escape = true;
		}
#endif /* _WIN32 */

		if (ch == '#' || ch == '&' || ch == ';' || ch == '`' || ch == '|' ||
			ch == '*' || ch == '?' || ch == '~' || ch == '<' || ch == '>' ||
			ch == '^' || ch == '(' || ch == ')' || ch == '[' || ch == ']' ||
			ch == '{' || ch == '}' || ch == '$' || ch == '\\' || ch == '\x0A' ||
			ch == '\xFF')
			escape = true;

		if (escape)
#ifdef _WIN32
			result += '^';
#else /* _WIN32 */
			result += '\\';
#endif /* _WIN32 */

		result += ch;
	}

	return result;
}

String Utility::EscapeShellArg(const String& s)
{
	String result;

#ifdef _WIN32
	result = "\"";
#else /* _WIN32 */
	result = "'";
#endif /* _WIN32 */

	for (char ch : s) {
#ifdef _WIN32
		if (ch == '"' || ch == '%') {
			result += ' ';
		}
#else /* _WIN32 */
		if (ch == '\'')
			result += "'\\'";
#endif
		result += ch;
	}

#ifdef _WIN32
	result += '"';
#else /* _WIN32 */
	result += '\'';
#endif /* _WIN32 */

	return result;
}

#ifdef _WIN32
String Utility::EscapeCreateProcessArg(const String& arg)
{
	if (arg.FindFirstOf(" \t\n\v\"") == String::NPos)
		return arg;

	String result = "\"";

	for (String::ConstIterator it = arg.Begin(); ; it++) {
		int numBackslashes = 0;

		while (it != arg.End() && *it == '\\') {
			it++;
			numBackslashes++;
		}

		if (it == arg.End()) {
			result.Append(numBackslashes * 2, '\\');
			break;
		} else if (*it == '"') {
			result.Append(numBackslashes * 2, '\\');
			result.Append(1, *it);
		} else {
			result.Append(numBackslashes, '\\');
			result.Append(1, *it);
		}
	}

	result += "\"";

	return result;
}
#endif /* _WIN32 */

#ifdef _WIN32
static void WindowsSetThreadName(const char *name)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = -1;
	info.dwFlags = 0;

	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		/* Nothing to do here. */
	}
}
#endif /* _WIN32 */

void Utility::SetThreadName(const String& name, bool os)
{
	m_ThreadName.reset(new String(name));

	if (!os)
		return;

#ifdef _WIN32
	WindowsSetThreadName(name.CStr());
#endif /* _WIN32 */

#ifdef HAVE_PTHREAD_SET_NAME_NP
	pthread_set_name_np(pthread_self(), name.CStr());
#endif /* HAVE_PTHREAD_SET_NAME_NP */

#ifdef HAVE_PTHREAD_SETNAME_NP
#	ifdef __APPLE__
	pthread_setname_np(name.CStr());
#	else /* __APPLE__ */
	String tname = name.SubStr(0, 15);
	pthread_setname_np(pthread_self(), tname.CStr());
#	endif /* __APPLE__ */
#endif /* HAVE_PTHREAD_SETNAME_NP */
}

String Utility::GetThreadName()
{
	String *name = m_ThreadName.get();

	if (!name) {
		std::ostringstream idbuf;
		idbuf << std::this_thread::get_id();
		return idbuf.str();
	}

	return *name;
}

unsigned long Utility::SDBM(const String& str, size_t len)
{
	unsigned long hash = 0;
	size_t current = 0;

	for (char c : str) {
		if (current >= len)
			break;

		hash = c + (hash << 6) + (hash << 16) - hash;

		current++;
	}

	return hash;
}

int Utility::CompareVersion(const String& v1, const String& v2)
{
	std::vector<String> tokensv1 = v1.Split(".");
	std::vector<String> tokensv2 = v2.Split(".");

	for (std::vector<String>::size_type i = 0; i < tokensv2.size() - tokensv1.size(); i++)
		tokensv1.emplace_back("0");

	for (std::vector<String>::size_type i = 0; i < tokensv1.size() - tokensv2.size(); i++)
		tokensv2.emplace_back("0");

	for (std::vector<String>::size_type i = 0; i < tokensv1.size(); i++) {
		if (Convert::ToLong(tokensv2[i]) > Convert::ToLong(tokensv1[i]))
			return 1;
		else if (Convert::ToLong(tokensv2[i]) < Convert::ToLong(tokensv1[i]))
			return -1;
	}

	return 0;
}

String Utility::GetHostName()
{
	char name[255];

	if (gethostname(name, sizeof(name)) < 0)
		return "localhost";

	return name;
}

/**
 * Returns the fully-qualified domain name for the host
 * we're running on.
 *
 * @returns The FQDN.
 */
String Utility::GetFQDN()
{
	String hostname = GetHostName();

	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_CANONNAME;

	addrinfo *result;
	int rc = getaddrinfo(hostname.CStr(), nullptr, &hints, &result);

	if (rc != 0)
		result = nullptr;

	if (result) {
		if (strcmp(result->ai_canonname, "localhost") != 0)
			hostname = result->ai_canonname;

		freeaddrinfo(result);
	}

	return hostname;
}

int Utility::Random()
{
#ifdef _WIN32
	return rand();
#else /* _WIN32 */
	unsigned int *seed = m_RandSeed.get();

	if (!seed) {
		seed = new unsigned int(Utility::GetTime());
		m_RandSeed.reset(seed);
	}

	return rand_r(seed);
#endif /* _WIN32 */
}

tm Utility::LocalTime(time_t ts)
{
#ifdef _MSC_VER
	tm *result = localtime(&ts);

	if (!result) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("localtime")
			<< boost::errinfo_errno(errno));
	}

	return *result;
#else /* _MSC_VER */
	tm result;

	if (!localtime_r(&ts, &result)) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("localtime_r")
			<< boost::errinfo_errno(errno));
	}

	return result;
#endif /* _MSC_VER */
}

bool Utility::PathExists(const String& path)
{
	namespace fs = boost::filesystem;

	boost::system::error_code ec;

	return fs::exists(fs::path(path.Begin(), path.End()), ec) && !ec;
}

Value Utility::LoadJsonFile(const String& path)
{
	std::ifstream fp;
	fp.open(path.CStr());

	String json((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());

	fp.close();

	if (fp.fail())
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not read JSON file '" + path + "'."));

	return JsonDecode(json);
}

void Utility::SaveJsonFile(const String& path, int mode, const Value& value)
{
	namespace fs = boost::filesystem;

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(path + ".XXXXXX", mode, fp);

	fp.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	fp << JsonEncode(value);
	fp.close();

	RenameFile(tempFilename, path);
}

static void HexEncode(char ch, std::ostream& os)
{
	const char *hex_chars = "0123456789ABCDEF";

	os << hex_chars[ch >> 4 & 0x0f];
	os << hex_chars[ch & 0x0f];
}

static int HexDecode(char hc)
{
	if (hc >= '0' && hc <= '9')
		return hc - '0';
	else if (hc >= 'a' && hc <= 'f')
		return hc - 'a' + 10;
	else if (hc >= 'A' && hc <= 'F')
		return hc - 'A' + 10;
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid hex character."));
}

String Utility::EscapeString(const String& s, const String& chars, const bool illegal)
{
	std::ostringstream result;
	if (illegal) {
		for (char ch : s) {
			if (chars.FindFirstOf(ch) != String::NPos || ch == '%') {
				result << '%';
				HexEncode(ch, result);
			} else
				result << ch;
		}
	} else {
		for (char ch : s) {
			if (chars.FindFirstOf(ch) == String::NPos || ch == '%') {
				result << '%';
				HexEncode(ch, result);
			} else
				result << ch;
		}
	}

	return result.str();
}

String Utility::UnescapeString(const String& s)
{
	std::ostringstream result;

	for (String::SizeType i = 0; i < s.GetLength(); i++) {
		if (s[i] == '%') {
			if (i + 2 > s.GetLength() - 1)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid escape sequence."));

			char ch = HexDecode(s[i + 1]) * 16 + HexDecode(s[i + 2]);
			result << ch;

			i += 2;
		} else
			result << s[i];
	}

	return result.str();
}

#ifndef _WIN32
static String UnameHelper(char type)
{
	struct utsname name;
	uname(&name);

	switch (type) {
		case 'm':
			return (char*)name.machine;
		case 'n':
			return (char*)name.nodename;
		case 'r':
			return (char*)name.release;
		case 's':
			return (char*)name.sysname;
		case 'v':
			return (char*)name.version;
		default:
			VERIFY(!"Invalid uname query.");
	}
}
#endif /* _WIN32 */
static bool ReleaseHelper(String *platformName, String *platformVersion)
{
#ifdef _WIN32
	if (platformName)
		*platformName = "Windows";

	if (platformVersion) {
		*platformVersion = "Vista";
		if (IsWindowsVistaSP1OrGreater())
			*platformVersion = "Vista SP1";
		if (IsWindowsVistaSP2OrGreater())
			*platformVersion = "Vista SP2";
		if (IsWindows7OrGreater())
			*platformVersion = "7";
		if (IsWindows7SP1OrGreater())
			*platformVersion = "7 SP1";
		if (IsWindows8OrGreater())
			*platformVersion = "8";
		if (IsWindows8Point1OrGreater())
			*platformVersion = "8.1 or greater";
		if (IsWindowsServer())
			*platformVersion += " (Server)";
	}

	return true;
#else /* _WIN32 */
	if (platformName)
		*platformName = "Unknown";

	if (platformVersion)
		*platformVersion = "Unknown";

	/* You have systemd or Ubuntu etc. */
	std::ifstream release("/etc/os-release");
	if (release.is_open()) {
		std::string release_line;
		while (getline(release, release_line)) {
			std::string::size_type pos = release_line.find("=");

			if (pos == std::string::npos)
				continue;

			std::string key = release_line.substr(0, pos);
			std::string value = release_line.substr(pos + 1);

			std::string::size_type firstQuote = value.find("\"");

			if (firstQuote != std::string::npos)
				value.erase(0, firstQuote + 1);

			std::string::size_type lastQuote = value.rfind("\"");

			if (lastQuote != std::string::npos)
				value.erase(lastQuote);

			if (platformName && key == "NAME")
				*platformName = value;

			if (platformVersion && key == "VERSION")
				*platformVersion = value;
		}

		return true;
	}

	/* You are using a distribution which supports LSB. */
	FILE *fp = popen("type lsb_release >/dev/null 2>&1 && lsb_release -s -i 2>&1", "r");

	if (fp) {
		std::ostringstream msgbuf;
		char line[1024];
		while (fgets(line, sizeof(line), fp))
			msgbuf << line;
		int status = pclose(fp);
		if (WEXITSTATUS(status) == 0) {
			if (platformName)
				*platformName = msgbuf.str();
		}
	}

	fp = popen("type lsb_release >/dev/null 2>&1 && lsb_release -s -r 2>&1", "r");

	if (fp) {
		std::ostringstream msgbuf;
		char line[1024];
		while (fgets(line, sizeof(line), fp))
			msgbuf << line;
		int status = pclose(fp);
		if (WEXITSTATUS(status) == 0) {
			if (platformVersion)
				*platformVersion = msgbuf.str();
		}
	}

	/* OS X */
	fp = popen("type sw_vers >/dev/null 2>&1 && sw_vers -productName 2>&1", "r");

	if (fp) {
		std::ostringstream msgbuf;
		char line[1024];
		while (fgets(line, sizeof(line), fp))
			msgbuf << line;
		int status = pclose(fp);
		if (WEXITSTATUS(status) == 0) {
			String info = msgbuf.str();
			info = info.Trim();

			if (platformName)
				*platformName = info;
		}
	}

	fp = popen("type sw_vers >/dev/null 2>&1 && sw_vers -productVersion 2>&1", "r");

	if (fp) {
		std::ostringstream msgbuf;
		char line[1024];
		while (fgets(line, sizeof(line), fp))
			msgbuf << line;
		int status = pclose(fp);
		if (WEXITSTATUS(status) == 0) {
			String info = msgbuf.str();
			info = info.Trim();

			if (platformVersion)
				*platformVersion = info;

			return true;
		}
	}

	/* Centos/RHEL < 7 */
	release.close();
	release.open("/etc/redhat-release");
	if (release.is_open()) {
		std::string release_line;
		getline(release, release_line);

		String info = release_line;

		/* example: Red Hat Enterprise Linux Server release 6.7 (Santiago) */
		if (platformName)
			*platformName = info.SubStr(0, info.Find("release") - 1);

		if (platformVersion)
			*platformVersion = info.SubStr(info.Find("release") + 8);

		return true;
	}

	/* sles 11 sp3, opensuse w/e */
	release.close();
	release.open("/etc/SuSE-release");
	if (release.is_open()) {
		std::string release_line;
		getline(release, release_line);

		String info = release_line;

		if (platformName)
			*platformName = info.SubStr(0, info.FindFirstOf(" "));

		if (platformVersion)
			*platformVersion = info.SubStr(info.FindFirstOf(" ") + 1);

		return true;
	}

	/* Just give up */
	return false;
#endif /* _WIN32 */
}

String Utility::GetPlatformKernel()
{
#ifdef _WIN32
	return "Windows";
#else /* _WIN32 */
	return UnameHelper('s');
#endif /* _WIN32 */
}

String Utility::GetPlatformKernelVersion()
{
#ifdef _WIN32
	OSVERSIONINFO info;
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&info);

	std::ostringstream msgbuf;
	msgbuf << info.dwMajorVersion << "." << info.dwMinorVersion;

	return msgbuf.str();
#else /* _WIN32 */
	return UnameHelper('r');
#endif /* _WIN32 */
}

String Utility::GetPlatformName()
{
	String platformName;
	if (!ReleaseHelper(&platformName, nullptr))
		return "Unknown";
	return platformName;
}

String Utility::GetPlatformVersion()
{
	String platformVersion;
	if (!ReleaseHelper(nullptr, &platformVersion))
		return "Unknown";
	return platformVersion;
}

String Utility::GetPlatformArchitecture()
{
#ifdef _WIN32
	SYSTEM_INFO info;
	GetNativeSystemInfo(&info);
	switch (info.wProcessorArchitecture) {
		case PROCESSOR_ARCHITECTURE_AMD64:
			return "x86_64";
		case PROCESSOR_ARCHITECTURE_ARM:
			return "arm";
		case PROCESSOR_ARCHITECTURE_INTEL:
			return "x86";
		default:
			return "unknown";
	}
#else /* _WIN32 */
	return UnameHelper('m');
#endif /* _WIN32 */
}

const char l_Utf8Replacement[] = "\xEF\xBF\xBD";

String Utility::ValidateUTF8(const String& input)
{
	std::vector<char> output;
	output.reserve(input.GetLength() * 3u);

	try {
		utf8::replace_invalid(input.Begin(), input.End(), std::back_inserter(output));
	} catch (const utf8::not_enough_room&) {
		output.insert(output.end(), (const char*)l_Utf8Replacement, (const char*)l_Utf8Replacement + 3);
	}

	return String(output.begin(), output.end());
}

String Utility::CreateTempFile(const String& path, int mode, std::fstream& fp)
{
	std::vector<char> targetPath(path.Begin(), path.End());
	targetPath.push_back('\0');

	int fd;
#ifndef _WIN32
	fd = mkstemp(&targetPath[0]);
#else /* _WIN32 */
	fd = MksTemp(&targetPath[0]);
#endif /*_WIN32*/

	if (fd < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("mkstemp")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(path));
	}

	try {
		fp.open(&targetPath[0], std::ios_base::trunc | std::ios_base::out);
	} catch (const std::fstream::failure&) {
		close(fd);
		throw;
	}

	close(fd);

	String resultPath = String(targetPath.begin(), targetPath.end() - 1);

	if (chmod(resultPath.CStr(), mode) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("chmod")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(resultPath));
	}

	return resultPath;
}

#ifdef _WIN32
/* mkstemp extracted from libc/sysdeps/posix/tempname.c.  Copyright
 * (C) 1991-1999, 2000, 2001, 2006 Free Software Foundation, Inc.
 *
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#define _O_EXCL 0x0400
#define _O_CREAT 0x0100
#define _O_RDWR 0x0002
#define O_EXCL _O_EXCL
#define O_CREAT	_O_CREAT
#define O_RDWR _O_RDWR

static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/* Generate a temporary file name based on TMPL.  TMPL must match the
 * rules for mk[s]temp (i.e. end in "XXXXXX").  The name constructed
 * does not exist at the time of the call to mkstemp.  TMPL is
 * overwritten with the result.
 */
int Utility::MksTemp(char *tmpl)
{
	int len;
	char *XXXXXX;
	static unsigned long long value;
	unsigned long long random_time_bits;
	unsigned int count;
	int fd = -1;
	int save_errno = errno;

	/* A lower bound on the number of temporary files to attempt to
	 * generate.  The maximum total number of temporary file names that
	 * can exist for a given template is 62**6.  It should never be
	 * necessary to try all these combinations.  Instead if a reasonable
	 * number of names is tried (we define reasonable as 62**3) fail to
	 * give the system administrator the chance to remove the problems.
	 */
#define ATTEMPTS_MIN (62 * 62 * 62)

	/* The number of times to attempt to generate a temporary file
	 * To conform to POSIX, this must be no smaller than TMP_MAX.
	 */
#if ATTEMPTS_MIN < TMP_MAX
	unsigned int attempts = TMP_MAX;
#else
	unsigned int attempts = ATTEMPTS_MIN;
#endif

	len = strlen (tmpl);
	if (len < 6 || strcmp (&tmpl[len - 6], "XXXXXX")) {
		errno = EINVAL;
		return -1;
	}

	/* This is where the Xs start.  */
	XXXXXX = &tmpl[len - 6];

	/* Get some more or less random data.  */
	{
		SYSTEMTIME stNow;
		FILETIME ftNow;

		// get system time
		GetSystemTime(&stNow);
		stNow.wMilliseconds = 500;
		if (!SystemTimeToFileTime(&stNow, &ftNow)) {
			errno = -1;
			return -1;
		}

		random_time_bits = (((unsigned long long)ftNow.dwHighDateTime << 32) | (unsigned long long)ftNow.dwLowDateTime);
	}

	value += random_time_bits ^ (unsigned long long)GetCurrentThreadId();

	for (count = 0; count < attempts; value += 7777, ++count) {
		unsigned long long v = value;

		/* Fill in the random bits.  */
		XXXXXX[0] = letters[v % 62];
		v /= 62;
		XXXXXX[1] = letters[v % 62];
		v /= 62;
		XXXXXX[2] = letters[v % 62];
		v /= 62;
		XXXXXX[3] = letters[v % 62];
		v /= 62;
		XXXXXX[4] = letters[v % 62];
		v /= 62;
		XXXXXX[5] = letters[v % 62];

		fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL, _S_IREAD | _S_IWRITE);
		if (fd >= 0) {
			errno = save_errno;
			return fd;
		} else if (errno != EEXIST)
			return -1;
	}

	/* We got out of the loop because we ran out of combinations to try.  */
	errno = EEXIST;
	return -1;
}

String Utility::GetIcingaInstallPath()
{
	char szProduct[39];

	for (int i = 0; MsiEnumProducts(i, szProduct) == ERROR_SUCCESS; i++) {
		char szName[128];
		DWORD cbName = sizeof(szName);
		if (MsiGetProductInfo(szProduct, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, szName, &cbName) != ERROR_SUCCESS)
			continue;

		if (strcmp(szName, "Icinga 2") != 0)
			continue;

		char szLocation[1024];
		DWORD cbLocation = sizeof(szLocation);
		if (MsiGetProductInfo(szProduct, INSTALLPROPERTY_INSTALLLOCATION, szLocation, &cbLocation) == ERROR_SUCCESS)
			return szLocation;
	}

	return "";
}

String Utility::GetIcingaDataPath()
{
	char path[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, path)))
		return "";
	return String(path) + "\\icinga2";
}

#endif /* _WIN32 */

/**
 * Retrieve the environment variable value by given key.
 *
 * @param env Environment variable name.
 */

String Utility::GetFromEnvironment(const String& env)
{
	const char *envValue = getenv(env.CStr());

	if (envValue == NULL)
		return String();
	else
		return String(envValue);
}

/**
 * Compare the password entered by a client with the actual password.
 * The comparision is safe against timing attacks.
 */
bool Utility::ComparePasswords(const String& enteredPassword, const String& actualPassword)
{
	volatile const char * volatile enteredPasswordCStr = enteredPassword.CStr();
	volatile size_t enteredPasswordLen = enteredPassword.GetLength();

	volatile const char * volatile actualPasswordCStr = actualPassword.CStr();
	volatile size_t actualPasswordLen = actualPassword.GetLength();

	volatile uint_fast8_t result = enteredPasswordLen == actualPasswordLen;

	if (result) {
		auto cStr (actualPasswordCStr);
		auto len (actualPasswordLen);

		actualPasswordCStr = cStr;
		actualPasswordLen = len;
	} else {
		auto cStr (enteredPasswordCStr);
		auto len (enteredPasswordLen);

		actualPasswordCStr = cStr;
		actualPasswordLen = len;
	}

	for (volatile size_t i = 0; i < enteredPasswordLen; ++i) {
		result &= uint_fast8_t(enteredPasswordCStr[i] == actualPasswordCStr[i]);
	}

	return result;
}
