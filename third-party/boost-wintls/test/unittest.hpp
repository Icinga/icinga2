//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_WINTLS_UNITTEST_HPP
#define BOOST_WINTLS_UNITTEST_HPP

#include <boost/wintls/detail/config.hpp>

// Workaround missing include in boost 1.76 and 1.77 in beast::test::stream
#if (BOOST_VERSION / 100 % 1000) == 77 || (BOOST_VERSION / 100 % 1000) == 76
#include <boost/make_shared.hpp>
#endif

#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/asio/ssl.hpp>

#include <catch2/catch.hpp>

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

namespace Catch {
template<>
struct StringMaker<boost::system::error_code> {
  static std::string convert(const boost::system::error_code& ec) {
    std::ostringstream oss;
    oss << ec.message() << " (0x" << std::hex << ec.value() << ")";
    return oss.str();
  }
};
}

inline std::vector<unsigned char> bytes_from_file(const std::string& path) {
  std::ifstream ifs{path};
  if (ifs.fail()) {
    throw std::runtime_error("Failed to open file " + path);
  }
  return {std::istreambuf_iterator<char>{ifs}, {}};
}

namespace net = boost::wintls::net;
namespace asio_ssl = boost::asio::ssl;
using test_stream = boost::beast::test::stream;

#endif // BOOST_WINTLS_UNITTEST_HPP
