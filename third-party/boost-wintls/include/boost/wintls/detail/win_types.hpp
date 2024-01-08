//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_WIN_TYPES_H
#define BOOST_WINTLS_DETAIL_WIN_TYPES_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define BOOST_WINTLS_WIN32_LEAN_AND_MEAN_DEFINED
#endif // WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#define BOOST_WINTLS_NOMINMAX_DEFINED
#endif // NOMINMAX

#include <windows.h>

#ifdef BOOST_WINTLS_WIN32_LEAN_AND_MEAN_DEFINED
#undef WIN32_LEAN_AND_MEAN
#endif // BOOST_WINTLS_WIN32_LEAN_AND_MEAN_DEFINED

#ifdef BOOST_WINTLS_NOMINMAX_DEFINED
#undef NOMINMAX
#endif // BOOST_WINTLS_NOMINMAX_DEFINED

#endif // BOOST_WINTLS_DETAIL_WIN_TYPES_H
