//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_WINTLS_TEST_TLS_RECORD_HPP
#define BOOST_WINTLS_TEST_TLS_RECORD_HPP

#include "unittest.hpp"

#include <boost/variant.hpp>

#include <cstdint>

enum class tls_version : std::uint16_t {
  ssl_3_0 = 0x0300,
  tls_1_0 = 0x0301,
  tls_1_1 = 0x0302,
  tls_1_2 = 0x0303,
  tls_1_3 = 0x0304
};

struct tls_change_cipher_spec {
  // TODO: Implement
};

struct tls_alert {
  // TODO: Implement
};

struct tls_handshake {
  enum class handshake_type : std::uint8_t {
    hello_request = 0x00,
    client_hello = 0x01,
    server_hello = 0x02,
    certificate = 0x0b,
    server_key_exchange = 0x0c,
    certificate_request = 0x0d,
    server_done = 0x0e,
    certificate_verify = 0x0f,
    client_key_exchange = 0x10,
    finished = 0x14
  };

  struct hello_request {
    // TODO: Implement
  };

  struct client_hello {
    // TODO: Implement
  };

  struct server_hello {
    // TODO: Implement
  };

  struct certificate {
    // TODO: Implement
  };

  struct server_key_exchange {
    // TODO: Implement
  };

  struct certificate_request {
    // TODO: Implement
  };

  struct server_done {
    // TODO: Implement
  };

  struct certificate_verify {
    // TODO: Implement
  };

  struct client_key_exchange {
    // TODO: Implement
  };

  struct finished {
    // TODO: Implement
  };

  using message_type = boost::variant<hello_request,
                                      client_hello,
                                      server_hello,
                                      certificate,
                                      server_key_exchange,
                                      certificate_request,
                                      server_done,
                                      certificate_verify,
                                      client_key_exchange,
                                      finished>;

  tls_handshake(net::const_buffer data);

  handshake_type type;
  std::uint32_t size;
  message_type message;
};

struct tls_application_data {
  // TODO: Implement
};

struct tls_record {
  enum class record_type : std::uint8_t {
    change_cipher_spec = 0x14,
    alert = 0x15,
    handshake = 0x16,
    application_data = 0x17
  };

  using message_type = boost::variant<tls_change_cipher_spec,
                                      tls_alert,
                                      tls_handshake,
                                      tls_application_data>;
  tls_record(net::const_buffer data);

  record_type type;
  tls_version version;
  std::uint16_t size;
  message_type message;
};

#endif // BOOST_WINTLS_TEST_TLS_RECORD_HPP
