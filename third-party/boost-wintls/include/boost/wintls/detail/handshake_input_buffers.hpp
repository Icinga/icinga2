//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_HANDSHAKE_INPUT_BUFFERS_HPP
#define BOOST_WINTLS_DETAIL_HANDSHAKE_INPUT_BUFFERS_HPP

#include <boost/wintls/detail/sspi_buffer_sequence.hpp>

namespace boost {
namespace wintls {
namespace detail {

class handshake_input_buffers : public sspi_buffer_sequence<2> {
public:
  handshake_input_buffers()
    : sspi_buffer_sequence(std::array<sspi_buffer, 2> {
        SECBUFFER_TOKEN,
        SECBUFFER_EMPTY
      }) {
  }
};

} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_HANDSHAKE_INPUT_BUFFERS_HPP
