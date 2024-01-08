//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_SSPI_TYPES_HPP
#define BOOST_WINTLS_DETAIL_SSPI_TYPES_HPP

#include <boost/wintls/detail/win_types.hpp>

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#define BOOST_WINTLS_SECURITY_WIN32_DEFINED
#endif // SECURITY_WIN32

#ifdef UNICODE
#undef UNICODE
#define BOOST_WINTLS_UNICODE_UNDEFINED
#endif // UNICODE

#include <schannel.h>
#include <security.h>

#ifdef BOOST_WINTLS_SECURITY_WIN32_DEFINED
#undef SECURITY_WIN32
#endif // BOOST_WINTLS_SECURITY_WIN32_DEFINED

#ifdef BOOST_WINTLS_UNICODE_UNDEFINED
#define UNICODE
#endif // BOOST_WINTLS_UNICODE_UNDEFINED

#endif // BOOST_WINTLS_DETAIL_SSPI_TYPES_HPP
