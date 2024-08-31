//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_CONFIG_HPP
#define BOOST_WINTLS_DETAIL_CONFIG_HPP

#include <boost/config.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif
#include <boost/asio.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#if !defined(__MINGW32__)
#pragma comment(lib, "crypt32")
#pragma comment(lib, "secur32")
#endif // __MINGW32__

namespace boost {
namespace wintls {

namespace net = boost::asio;

} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_CONFIG_HPP
