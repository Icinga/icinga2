//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_SSPI_SEC_HANDLE_HPP
#define BOOST_WINTLS_DETAIL_SSPI_SEC_HANDLE_HPP

#include <boost/wintls/detail/sspi_functions.hpp>

namespace boost {
namespace wintls {
namespace detail {

template <typename T>
class sspi_sec_handle {

public:
  sspi_sec_handle() = default;
  sspi_sec_handle(sspi_sec_handle&&) = delete;
  sspi_sec_handle& operator=(sspi_sec_handle&&) = delete;

  operator bool() {
    return handle_.dwLower != 0 || handle_.dwUpper != 0;
  }

  T* get() {
    return &handle_;
  }

private:
  T handle_{0, 0};
};

class ctxt_handle : public sspi_sec_handle<CtxtHandle> {
public:
  ~ctxt_handle() {
    if (*this) {
      detail::sspi_functions::DeleteSecurityContext(get());
    }
  }
};

class cred_handle : public sspi_sec_handle<CredHandle> {
public:
  ~cred_handle() {
    if (*this) {
      detail::sspi_functions::FreeCredentialsHandle(get());
    }
  }
};

} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_SSPI_SEC_HANDLE_HPP
