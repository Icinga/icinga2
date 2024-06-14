//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_TEST_ASYNC_ECHO_CLIENT_HPP
#define BOOST_WINTLS_TEST_ASYNC_ECHO_CLIENT_HPP

#include "unittest.hpp"

template<typename Stream>
struct async_echo_client : public Stream {
public:
  using Stream::stream;

  async_echo_client(net::io_context& context, const std::string& message)
    : Stream(context)
    , message_(message) {
  }

  void run() {
    do_handshake();
  }

  std::string received_message() const {
    return std::string(net::buffers_begin(recv_buffer_.data()),
                       net::buffers_begin(recv_buffer_.data()) + static_cast<std::ptrdiff_t>(recv_buffer_.size()));
  }

private:
  void do_handshake() {
    stream.async_handshake(Stream::handshake_type::client,
                           [this](const boost::system::error_code& ec) {
                             REQUIRE_FALSE(ec);
                             do_write();
                           });
  }

  void do_write() {
    net::async_write(stream, net::buffer(message_),
                     [this](const boost::system::error_code& ec, std::size_t) {
                       REQUIRE_FALSE(ec);
                       do_read();
                     });
  }

  void do_read() {
    net::async_read_until(stream, recv_buffer_, '\0',
                          [this](const boost::system::error_code& ec, std::size_t) {
                            REQUIRE_FALSE(ec);
                            do_shutdown();
                          });
  }

  void do_shutdown() {
    stream.async_shutdown([](const boost::system::error_code& ec) {
      REQUIRE_FALSE(ec);
    });
  }

  std::string message_;
  net::streambuf recv_buffer_;
};

#endif // BOOST_WINTLS_TEST_ASYNC_ECHO_CLIENT_HPP
