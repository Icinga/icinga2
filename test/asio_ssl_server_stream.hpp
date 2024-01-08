//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef ASIO_SSL_SERVER_STREAM_HPP
#define ASIO_SSL_SERVER_STREAM_HPP

#include "certificate.hpp"
#include "unittest.hpp"

struct asio_ssl_server_context : public asio_ssl::context {
  asio_ssl_server_context()
    : asio_ssl::context(asio_ssl::context_base::tls) {
    use_certificate(net::buffer(test_certificate), asio_ssl::context_base::pem);
    use_private_key(net::buffer(test_key), asio_ssl::context_base::pem);
  }

  void enable_client_verify() {
    set_verify_mode(asio_ssl::verify_peer | asio_ssl::verify_fail_if_no_peer_cert);
    add_certificate_authority(net::buffer(test_certificate));
  }
};

struct asio_ssl_server_stream {
  using handshake_type = asio_ssl::stream_base::handshake_type;

  template <class... Args>
  asio_ssl_server_stream(Args&&... args)
    : tst(std::forward<Args>(args)...)
    , stream(tst, ctx) {
  }

  asio_ssl_server_context ctx;
  test_stream tst;
  asio_ssl::stream<test_stream&> stream;
};

#endif // ASIO_SSL_SERVER_STREAM_HPP
