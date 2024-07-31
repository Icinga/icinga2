//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_SHUTDOWN_BUFFERS_HPP
#define BOOST_WINTLS_DETAIL_SHUTDOWN_BUFFERS_HPP

#include <boost/wintls/detail/sspi_buffer_sequence.hpp>

#include <cstddef>

namespace boost {
namespace wintls {
namespace detail {

class shutdown_buffers : public sspi_buffer_sequence<1> {
public:
  shutdown_buffers()
    : sspi_buffer_sequence(std::array<sspi_buffer, 1> {
        SECBUFFER_TOKEN
      }) {

    buffers_[0].pvBuffer = &shutdown_type;
    buffers_[0].cbBuffer = sizeof(shutdown_type);
  }

private:
  std::uint32_t shutdown_type = SCHANNEL_SHUTDOWN;
};

} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_SHUTDOWN_BUFFERS_HPP
