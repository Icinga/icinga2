//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_ERROR_HPP
#define BOOST_WINTLS_ERROR_HPP

#include <boost/wintls/detail/error.hpp>
#include <boost/wintls/detail/sspi_types.hpp>

namespace boost {
namespace wintls {
namespace error {

inline boost::system::error_code make_error_code(SECURITY_STATUS sc) {
  return boost::system::error_code(static_cast<int>(sc), boost::system::system_category());
}

} // namespace error
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_ERROR_HPP
