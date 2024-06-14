//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_TEST_ECHO_CLIENT_HPP
#define BOOST_WINTLS_TEST_ECHO_CLIENT_HPP

#include "unittest.hpp"

template<typename Stream>
class echo_client : public Stream {
public:
  using Stream::stream;

  echo_client(net::io_context& context)
    : Stream(context) {
  }

  void handshake() {
    stream.handshake(Stream::handshake_type::client);
  }

  void shutdown() {
    stream.shutdown();
  }

  void read() {
    net::read_until(stream, buffer_, '\0');
  }

  template <typename T>
  void write(const T& data) {
    net::write(stream, net::buffer(data));
  }

  template <typename T>
  T data() const {
    return T(net::buffers_begin(buffer_.data()), net::buffers_begin(buffer_.data()) + static_cast<std::ptrdiff_t>(buffer_.size()));
  }

private:
  net::streambuf buffer_;
};

#endif // BOOST_WINTLS_TEST_ECHO_CLIENT_HPP
