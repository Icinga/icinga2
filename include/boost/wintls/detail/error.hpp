//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_ERROR_HPP
#define BOOST_WINTLS_DETAIL_ERROR_HPP

#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>

namespace boost {
namespace wintls {
namespace detail {

inline boost::system::error_code get_last_error() noexcept {
  return boost::system::error_code(static_cast<int>(GetLastError()), boost::system::system_category());
}

inline void throw_last_error(const char* msg) {
  throw boost::system::system_error(get_last_error(), msg);
}

inline void throw_last_error() {
  throw boost::system::system_error(get_last_error());
}

inline void throw_error(const boost::system::error_code& ec) {
  throw boost::system::system_error(ec);
}

inline void throw_error(const boost::system::error_code& ec, const char* msg) {
  throw boost::system::system_error(ec, msg);
}

} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_ERROR_HPP
