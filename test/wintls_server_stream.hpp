//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef WINTLS_SERVER_STREAM_HPP
#define WINTLS_SERVER_STREAM_HPP

#include "unittest.hpp"
#include "certificate.hpp"

#include <boost/wintls.hpp>

const std::string test_key_name_server = test_key_name + "-server";

struct wintls_server_context : public boost::wintls::context {
  wintls_server_context()
    : boost::wintls::context(boost::wintls::method::system_default)
    , needs_private_key_clean_up_(false) {
      // delete key in case last test run has dangling key.
      boost::system::error_code dummy;
      boost::wintls::delete_private_key(test_key_name_server, dummy);

      auto cert_ptr = x509_to_cert_context(net::buffer(test_certificate), boost::wintls::file_format::pem);
      boost::wintls::import_private_key(net::buffer(test_key), boost::wintls::file_format::pem, test_key_name_server);
      needs_private_key_clean_up_ = true;
      boost::wintls::assign_private_key(cert_ptr.get(), test_key_name_server);
      add_certificate_authority(cert_ptr.get());
      use_certificate(cert_ptr.get());
  }

  void enable_client_verify() {
    verify_server_certificate(true);
  }

  ~wintls_server_context() {
    if(needs_private_key_clean_up_) {
      boost::wintls::delete_private_key(test_key_name_server);
      needs_private_key_clean_up_ = false;
    }
  }

private:
  bool needs_private_key_clean_up_;
};

struct wintls_server_stream {
  using handshake_type = boost::wintls::handshake_type;

  template <class... Args>
  wintls_server_stream(Args&&... args)
    : tst(std::forward<Args>(args)...)
    , stream(tst, ctx) {
  }

  wintls_server_context ctx;
  test_stream tst;
  boost::wintls::stream<test_stream&> stream;
};

#endif // WINTLS_SERVER_STREAM_HPP
