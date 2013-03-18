/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "base/utility.h"
#include "base/application.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include <mmatch.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/foreach.hpp>

#if HAVE_GCC_ABI_DEMANGLE
#	include <cxxabi.h>
#endif /* HAVE_GCC_ABI_DEMANGLE */

using namespace icinga;

bool I2_EXPORT Utility::m_SSLInitialized = false;

/**
 * Demangles a symbol name.
 *
 * @param sym The symbol name.
 * @returns A human-readable version of the symbol name.
 */
String Utility::DemangleSymbolName(const String& sym)
{
	String result = sym;

#if HAVE_GCC_ABI_DEMANGLE
	int status;
	char *realname = abi::__cxa_demangle(sym.CStr(), 0, 0, &status);

	if (realname != NULL) {
		result = String(realname);
		free(realname);
	}
#endif /* HAVE_GCC_ABI_DEMANGLE */

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

/**
 * Initializes the OpenSSL library.
 */
void Utility::InitializeOpenSSL(void)
{
	if (m_SSLInitialized)
		return;

	SSL_library_init();
	SSL_load_error_strings();

	m_SSLInitialized = true;
}

/**
 * Initializes an SSL context using the specified certificates.
 *
 * @param pubkey The public key.
 * @param privkey The matching private key.
 * @param cakey CA certificate chain file.
 * @returns An SSL context.
 */
shared_ptr<SSL_CTX> Utility::MakeSSLContext(const String& pubkey, const String& privkey, const String& cakey)
{
	InitializeOpenSSL();

	shared_ptr<SSL_CTX> sslContext = shared_ptr<SSL_CTX>(SSL_CTX_new(TLSv1_method()), SSL_CTX_free);

	SSL_CTX_set_mode(sslContext.get(), SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	if (!SSL_CTX_use_certificate_chain_file(sslContext.get(), pubkey.CStr())) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_use_certificate_chain_file")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(pubkey));
	}

	if (!SSL_CTX_use_PrivateKey_file(sslContext.get(), privkey.CStr(), SSL_FILETYPE_PEM)) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_use_PrivateKey_file")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(privkey));
	}

	if (!SSL_CTX_load_verify_locations(sslContext.get(), cakey.CStr(), NULL)) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_load_verify_locations")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(cakey));
	}

	STACK_OF(X509_NAME) *cert_names;

	cert_names = SSL_load_client_CA_file(cakey.CStr());
	if (cert_names == NULL) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_load_client_CA_file")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(cakey));
	}

	SSL_CTX_set_client_CA_list(sslContext.get(), cert_names);

	return sslContext;
}

/**
 * Retrieves the common name for an X509 certificate.
 *
 * @param certificate The X509 certificate.
 * @returns The common name.
 */
String Utility::GetCertificateCN(const shared_ptr<X509>& certificate)
{
	char buffer[256];

	int rc = X509_NAME_get_text_by_NID(X509_get_subject_name(certificate.get()),
	    NID_commonName, buffer, sizeof(buffer));

	if (rc == -1) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("X509_NAME_get_text_by_NID")
		    << errinfo_openssl_error(ERR_get_error()));
	}

	return buffer;
}

/**
 * Retrieves an X509 certificate from the specified file.
 *
 * @param pemfile The filename.
 * @returns An X509 certificate.
 */
shared_ptr<X509> Utility::GetX509Certificate(const String& pemfile)
{
	X509 *cert;
	BIO *fpcert = BIO_new(BIO_s_file());

	if (fpcert == NULL) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("BIO_new")
		    << errinfo_openssl_error(ERR_get_error()));
	}

	if (BIO_read_filename(fpcert, pemfile.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("BIO_read_filename")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(pemfile));
	}

	cert = PEM_read_bio_X509_AUX(fpcert, NULL, NULL, NULL);
	if (cert == NULL) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("PEM_read_bio_X509_AUX")
		    << errinfo_openssl_error(ERR_get_error()));
	}

	BIO_free(fpcert);

	return shared_ptr<X509>(cert, X509_free);
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

/**
 * Returns the directory component of a path. See dirname(3) for details.
 *
 * @param path The full path.
 * @returns The directory.
 */
String Utility::DirName(const String& path)
{
	char *dir;

#ifdef _WIN32
	String dupPath = path;

	/* PathRemoveFileSpec doesn't properly handle forward slashes. */
	BOOST_FOREACH(char& ch, dupPath) {
		if (ch == '/')
			ch = '\\';
	}

	dir = strdup(dupPath.CStr());
#else /* _WIN32 */
	dir = strdup(path.CStr());
#endif /* _WIN32 */

	if (dir == NULL)
		BOOST_THROW_EXCEPTION(std::bad_alloc());

	String result;

#ifndef _WIN32
	result = dirname(dir);
#else /* _WIN32 */
	if (!PathRemoveFileSpec(dir)) {
		free(dir);

		BOOST_THROW_EXCEPTION(win32_error()
		    << boost::errinfo_api_function("PathRemoveFileSpec")
		    << errinfo_win32_error(GetLastError()));
	}

	result = dir;

	if (result.IsEmpty())
		result = ".";
#endif /* _WIN32 */

	free(dir);

	return result;
}

/**
 * Returns the file component of a path. See basename(3) for details.
 *
 * @param path The full path.
 * @returns The filename.
 */
String Utility::BaseName(const String& path)
{
	char *dir = strdup(path.CStr());
	String result;

	if (dir == NULL)
		BOOST_THROW_EXCEPTION(std::bad_alloc());

#ifndef _WIN32
	result = basename(dir);
#else /* _WIN32 */
	result = PathFindFileName(dir);
#endif /* _WIN32 */

	free(dir);

	return result;
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

/**
 * Returns the current UNIX timestamp including fractions of seconds.
 *
 * @returns The current time.
 */
double Utility::GetTime(void)
{
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

	if (gettimeofday(&tv, NULL) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("gettimeofday")
		    << boost::errinfo_errno(errno));
	}

	return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif /* _WIN32 */
}

/**
 * Returns the ID of the current process.
 *
 * @returns The PID.
 */
pid_t Utility::GetPid(void)
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
	usleep(timeout * 1000 * 1000);
#else /* _WIN32 */
	::Sleep(timeout * 1000);
#endif /* _WIN32 */
}

/**
 * Loads the specified library.
 *
 * @param library The name of the library.
 */
#ifdef _WIN32
HMODULE
#else /* _WIN32 */
lt_dlhandle
#endif /* _WIN32 */
Utility::LoadExtensionLibrary(const String& library)
{
	String path;
#ifdef _WIN32
	path = library + ".dll";
#else /* _WIN32 */
	path = "lib" + library + ".la";
#endif /* _WIN32 */

	Log(LogInformation, "base", "Loading library '" + path + "'");

#ifdef _WIN32
	HMODULE hModule = LoadLibrary(path.CStr());

	if (hModule == NULL) {
		BOOST_THROW_EXCEPTION(win32_error()
		    << boost::errinfo_api_function("LoadLibrary")
		    << errinfo_win32_error(GetLastError())
		    << boost::errinfo_file_name(path));
	}
#else /* _WIN32 */
	lt_dlhandle hModule = lt_dlopen(path.CStr());

	if (hModule == NULL) {
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not load library '" + path + "': " +  lt_dlerror()));
	}
#endif /* _WIN32 */

	return hModule;
}

/**
 * Generates a new UUID.
 *
 * @returns The new UUID in text form.
 */
String Utility::NewUUID(void)
{
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	return boost::lexical_cast<String>(uuid);
}

/**
 * Calls the specified callback for each file matching the path specification.
 *
 * @param pathSpec The path specification.
 */
bool Utility::Glob(const String& pathSpec, const boost::function<void (const String&)>& callback)
{
#ifdef _WIN32
	HANDLE handle;
	WIN32_FIND_DATA wfd;

	handle = FindFirstFile(pathSpec.CStr(), &wfd);

	if (handle == INVALID_HANDLE_VALUE) {
		DWORD errorCode = GetLastError();

		if (errorCode == ERROR_FILE_NOT_FOUND)
			return false;

		BOOST_THROW_EXCEPTION(win32_error()
		    << boost::errinfo_api_function("FindFirstFile")
			<< boost::errinfo_win32_error(errorCode)
		    << boost::errinfo_file_name(pathSpec));
	}

	do {
		callback(DirName(pathSpec) + "/" + wfd.cFileName);
	} while (FindNextFile(handle, &wfd));

	if (!FindClose(handle)) {
		BOOST_THROW_EXCEPTION(win32_error()
		    << boost::errinfo_api_function("FindClose")
		    << errinfo_win32_error(GetLastError()));
	}

	return true;
#else /* _WIN32 */
	glob_t gr;

	int rc = glob(pathSpec.CStr(), GLOB_ERR | GLOB_NOSORT, NULL, &gr);

	if (rc < 0) {
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
		callback(*gp);
	}

	globfree(&gr);

	return true;
#endif /* _WIN32 */
}

#ifndef _WIN32
void Utility::SetNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("fcntl")
		    << boost::errinfo_errno(errno));
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("fcntl")
		    << boost::errinfo_errno(errno));
	}
}

void Utility::SetCloExec(int fd)
{
	int flags = fcntl(fd, F_GETFD, 0);

	if (flags < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("fcntl")
		    << boost::errinfo_errno(errno));
	}

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("fcntl")
		    << boost::errinfo_errno(errno));
	}
}
#endif /* _WIN32 */

void Utility::SetNonBlockingSocket(SOCKET s)
{
#ifndef _WIN32
	SetNonBlocking(s);
#else /* _WIN32 */
	unsigned long lTrue = 1;
	ioctlsocket(s, FIONBIO, &lTrue);
#endif /* _WIN32 */
}

void Utility::QueueAsyncCallback(const boost::function<void (void)>& callback)
{
	Application::GetEQ().Post(callback);
}

String Utility::FormatDateTime(const char *format, double ts)
{
	char timestamp[128];
	time_t tempts = (time_t)ts; /* We don't handle sub-second timestamp here just yet. */
	tm tmthen;

#ifdef _MSC_VER
	tm *temp = localtime(&tempts);

	if (temp == NULL) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("localtime")
		    << boost::errinfo_errno(errno));
	}

	tmthen = *temp;
#else /* _MSC_VER */
	if (localtime_r(&tempts, &tmthen) == NULL) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("localtime_r")
		    << boost::errinfo_errno(errno));
	}
#endif /* _MSC_VER */

	strftime(timestamp, sizeof(timestamp), format, &tmthen);

	return timestamp;
}
