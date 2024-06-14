//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_TEST_ECHO_SERVER_HPP
#define BOOST_WINTLS_TEST_ECHO_SERVER_HPP

#include "unittest.hpp"

#include <future>

template<typename Stream>
class echo_server : public Stream {
public:
  using Stream::stream;

  echo_server(net::io_context& context)
    : Stream(context) {
  }

  std::future<boost::system::error_code> handshake() {
    return std::async(std::launch::async, [this]() {
      boost::system::error_code ec{};
      stream.handshake(Stream::handshake_type::server, ec);
      return ec;
    });
  }

  std::future<boost::system::error_code> shutdown() {
    return std::async(std::launch::async, [this]() {
      boost::system::error_code ec{};
      stream.shutdown(ec);
      return ec;
    });
  }

  void read() {
    net::read_until(stream, buffer_, '\0');
  }

  void write() {
    net::write(stream, buffer_);
  }

private:
  net::streambuf buffer_;
};

#endif // BOOST_WINTLS_TEST_ECHO_SERVER_HPP
