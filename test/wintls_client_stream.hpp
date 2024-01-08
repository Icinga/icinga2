//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef WINTLS_CLIENT_STREAM_HPP
#define WINTLS_CLIENT_STREAM_HPP

#include "certificate.hpp"
#include "unittest.hpp"

#include <boost/wintls.hpp>

#include <fstream>
#include <iterator>

const std::string test_key_name_client = test_key_name + "-client";

struct wintls_client_context : public boost::wintls::context {
  wintls_client_context()
    : boost::wintls::context(boost::wintls::method::system_default)
    , needs_private_key_clean_up_(false)
    , authority_ptr_() {
  }

  void with_test_cert_authority() {
    if(!authority_ptr_) {
      authority_ptr_ = x509_to_cert_context(net::buffer(test_certificate), boost::wintls::file_format::pem);
      add_certificate_authority(authority_ptr_.get());
    }
  }

  void with_test_client_cert() {
    with_test_cert_authority();

    // delete key in case last test run has dangling key.
    boost::system::error_code dummy;
    boost::wintls::delete_private_key(test_key_name_client, dummy);

    boost::wintls::import_private_key(net::buffer(test_key), boost::wintls::file_format::pem, test_key_name_client);
    needs_private_key_clean_up_ = true;
    boost::wintls::assign_private_key(authority_ptr_.get(), test_key_name_client);
    use_certificate(authority_ptr_.get());
  }

  void enable_server_verify() {
    verify_server_certificate(true);
  }

  ~wintls_client_context() {
    if(needs_private_key_clean_up_) {
      boost::wintls::delete_private_key(test_key_name_client);
      needs_private_key_clean_up_ = false;
    }
  }

private:
  bool needs_private_key_clean_up_;
  boost::wintls::cert_context_ptr authority_ptr_;
};

struct wintls_client_stream {
  using handshake_type = boost::wintls::handshake_type;

  template <class... Args>
  wintls_client_stream(Args&&... args)
    : tst(std::forward<Args>(args)...)
    , stream(tst, ctx) {
  }

  wintls_client_context ctx;
  test_stream tst;
  boost::wintls::stream<test_stream&> stream;
};

#endif // WINTLS_CLIENT_STREAM_HPP
