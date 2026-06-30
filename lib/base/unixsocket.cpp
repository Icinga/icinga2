// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/unixsocket.hpp"
#include "base/exception.hpp"

#ifndef _WIN32
using namespace icinga;

UnixSocket::UnixSocket()
{
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (fd < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("socket")
			<< boost::errinfo_errno(errno));
	}

	SetFD(fd);
}

void UnixSocket::Bind(const String& path)
{
	unlink(path.CStr());

	sockaddr_un s_un;
	memset(&s_un, 0, sizeof(s_un));
	s_un.sun_family = AF_UNIX;
	strncpy(s_un.sun_path, path.CStr(), sizeof(s_un.sun_path));
	s_un.sun_path[sizeof(s_un.sun_path) - 1] = '\0';

	if (bind(GetFD(), (sockaddr *)&s_un, SUN_LEN(&s_un)) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("bind")
			<< boost::errinfo_errno(errno));
	}
}

void UnixSocket::Connect(const String& path)
{
	sockaddr_un s_un;
	memset(&s_un, 0, sizeof(s_un));
	s_un.sun_family = AF_UNIX;
	strncpy(s_un.sun_path, path.CStr(), sizeof(s_un.sun_path));
	s_un.sun_path[sizeof(s_un.sun_path) - 1] = '\0';

	if (connect(GetFD(), (sockaddr *)&s_un, SUN_LEN(&s_un)) < 0 && errno != EINPROGRESS) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("connect")
			<< boost::errinfo_errno(errno));
	}
}
#endif /* _WIN32 */
