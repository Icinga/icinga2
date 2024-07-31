//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "unittest.hpp"

#include <boost/wintls/detail/sspi_buffer_sequence.hpp>

#include <iterator>

using boost::wintls::detail::sspi_buffer;
using boost::wintls::detail::sspi_buffer_sequence;

class test_buffer_sequence : public sspi_buffer_sequence<4> {
public:
  test_buffer_sequence()
    : sspi_buffer_sequence(std::array<sspi_buffer, 4> {
        SECBUFFER_EMPTY,
        SECBUFFER_EMPTY,
        SECBUFFER_EMPTY,
        SECBUFFER_EMPTY
      }) {
  }
};

static_assert(boost::wintls::net::is_const_buffer_sequence<test_buffer_sequence>::value,
              "ConstBufferSequence type requirements not met");

static_assert(boost::wintls::net::is_mutable_buffer_sequence<test_buffer_sequence>::value,
              "MutableBufferSequence type requirements not met");

TEST_CASE("sspi buffer sequence") {
  test_buffer_sequence sequence;
  CHECK(std::distance(sequence.begin(), sequence.end()) == 4);
  CHECK(std::all_of(sequence.begin(), sequence.end(), [](const auto& buffer) {
    return buffer.cbBuffer == 0;
  }));
  CHECK(std::all_of(sequence.begin(), sequence.end(), [](const auto& buffer) {
    return buffer.pvBuffer == nullptr;
  }));
}
