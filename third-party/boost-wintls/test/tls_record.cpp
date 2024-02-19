//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "tls_record.hpp"

namespace {
std::uint8_t net_to_host(std::uint8_t value) {
  return value;
}

std::uint16_t net_to_host(std::uint16_t value) {
  return ntohs(value);
}

std::uint32_t net_to_host(std::uint32_t value) {
  return ntohl(value);
}

template <typename SizeType>
SizeType read_value(net::const_buffer& buffer) {
  BOOST_ASSERT(buffer.size() >= sizeof(SizeType));
  SizeType ret = *reinterpret_cast<const SizeType*>(buffer.data());
  buffer += sizeof(SizeType);
  return net_to_host(ret);
}

std::uint32_t read_three_byte_value(net::const_buffer& buffer) {
  BOOST_ASSERT(buffer.size() >= 3);
  std::array<char, 4> value{};
  std::copy_n(reinterpret_cast<const char*>(buffer.data()), 3, value.begin() + 1);
  buffer += 3;
  return net_to_host(*reinterpret_cast<std::uint32_t*>(value.data()));
}

tls_record::message_type read_message(tls_record::record_type type, net::const_buffer& buffer) {
  switch(type) {
    case tls_record::record_type::change_cipher_spec:
      return tls_change_cipher_spec{};
    case tls_record::record_type::alert:
      return tls_alert{};
    case tls_record::record_type::handshake:
      return tls_handshake{buffer};
    case tls_record::record_type::application_data:
      return tls_application_data{};
  }
  BOOST_UNREACHABLE_RETURN(0);
}

tls_handshake::message_type read_message(tls_handshake::handshake_type t, net::const_buffer&) {
  switch(t) {
    case tls_handshake::handshake_type::hello_request:
      return tls_handshake::hello_request{};
    case tls_handshake::handshake_type::client_hello:
      return tls_handshake::client_hello{};
    case tls_handshake::handshake_type::server_hello:
      return tls_handshake::server_hello{};
    case tls_handshake::handshake_type::certificate:
      return tls_handshake::certificate{};
    case tls_handshake::handshake_type::server_key_exchange:
      return tls_handshake::server_key_exchange{};
    case tls_handshake::handshake_type::certificate_request:
      return tls_handshake::certificate_request{};
    case tls_handshake::handshake_type::server_done:
      return tls_handshake::server_done{};
    case tls_handshake::handshake_type::certificate_verify:
      return tls_handshake::certificate_verify{};
    case tls_handshake::handshake_type::client_key_exchange:
      return tls_handshake::client_key_exchange{};
    case tls_handshake::handshake_type::finished:
      return tls_handshake::finished{};
  }
  BOOST_UNREACHABLE_RETURN(0);
}

} // namespace

tls_handshake::tls_handshake(net::const_buffer data)
  : type(static_cast<handshake_type>(read_value<std::uint8_t>(data)))
  , size(read_three_byte_value(data))
  , message(read_message(type, data)) {
}

tls_record::tls_record(net::const_buffer data)
  : type(static_cast<record_type>(read_value<std::uint8_t>(data)))
  , version(static_cast<tls_version>(read_value<std::uint16_t>(data)))
  , size(read_value<std::uint16_t>(data))
  , message(read_message(type, data)) {
}
