//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "echo_server.hpp"
#include "echo_client.hpp"
#include "async_echo_server.hpp"
#include "async_echo_client.hpp"
#include "asio_ssl_client_stream.hpp"
#include "asio_ssl_server_stream.hpp"
#include "wintls_client_stream.hpp"
#include "wintls_server_stream.hpp"
#include "unittest.hpp"

#include <boost/wintls.hpp>

#include <cstdint>
#include <string>
#include <tuple>

namespace {
std::string generate_data(std::size_t size) {
  std::string ret(size, '\0');
  for (std::size_t i = 0; i < size - 1; ++i) {
    ret[i] = static_cast<char>(i % 26 + 65);
  }
  return ret;
}

}

using TestTypes = std::tuple<std::tuple<asio_ssl_client_stream, asio_ssl_server_stream>,
                             std::tuple<wintls_client_stream, asio_ssl_server_stream>,
                             std::tuple<asio_ssl_client_stream, wintls_server_stream>,
                             std::tuple<wintls_client_stream, wintls_server_stream>>;

TEMPLATE_LIST_TEST_CASE("echo test", "", TestTypes) {
  using ClientStream = typename std::tuple_element<0, TestType>::type;
  using ServerStream = typename std::tuple_element<1, TestType>::type;

  auto test_data_size = GENERATE(0x100, 0x100 - 1, 0x100 + 1,
                                 0x1000, 0x1000 - 1, 0x1000 + 1,
                                 0x10000, 0x10000 - 1, 0x10000 + 1,
                                 0x100000, 0x100000 - 1, 0x100000 + 1);
  const std::string test_data = generate_data(static_cast<std::size_t>(test_data_size));

  net::io_context io_context;

  SECTION("sync test") {
    echo_client<ClientStream> client(io_context);
    echo_server<ServerStream> server(io_context);

    client.stream.next_layer().connect(server.stream.next_layer());

    auto handshake_result = server.handshake();
    client.handshake();
    REQUIRE_FALSE(handshake_result.get());

    client.write(test_data);
    server.read();
    server.write();
    client.read();

    auto shutdown_result = server.shutdown();
    client.shutdown();
    REQUIRE_FALSE(shutdown_result.get());

    CHECK(client.template data<std::string>() == test_data);
  }

  SECTION("async test") {
    async_echo_server<ServerStream> server(io_context);
    async_echo_client<ClientStream> client(io_context, test_data);
    client.stream.next_layer().connect(server.stream.next_layer());
    server.run();
    client.run();
    io_context.run();
    CHECK(client.received_message() == test_data);
  }
}
