//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "unittest.hpp"

#include <boost/wintls/detail/decrypted_data_buffer.hpp>

#include <string>

TEST_CASE("decrypted data buffer") {
  boost::wintls::detail::decrypted_data_buffer<25> test_buffer;
  CHECK(test_buffer.empty());

  std::string input_str{"abc"};
  test_buffer.fill(net::buffer(input_str));
  CHECK_FALSE(test_buffer.empty());

  std::string output_str(1, '\0');

  test_buffer.get(net::buffer(output_str));
  CHECK_FALSE(test_buffer.empty());
  CHECK(output_str == "a");

  test_buffer.get(net::buffer(output_str));
  CHECK_FALSE(test_buffer.empty());
  CHECK(output_str == "b");

  test_buffer.get(net::buffer(output_str));
  CHECK(test_buffer.empty());
  CHECK(output_str == "c");

  test_buffer.fill(net::buffer(input_str));
  output_str = "defg";
  const auto size = test_buffer.get(net::buffer(output_str));
  CHECK(size == 3);
  CHECK(test_buffer.empty());
  CHECK(output_str == "abcg");
}
