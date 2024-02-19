//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "unittest.hpp"
#include "asio_ssl_server_stream.hpp"
#include "wintls_client_stream.hpp"
#include "wintls_server_stream.hpp"
#include "echo_client.hpp"
#include "echo_server.hpp"
#include "async_echo_client.hpp"
#include "async_echo_server.hpp"

#include <boost/wintls.hpp>
#include <boost/wintls/detail/config.hpp>

#include <boost/asio/io_context.hpp>

#include <array>
#include <thread>
#include <string>

class test_server : public async_echo_server<asio_ssl_server_stream> {
public:
  test_server(net::io_context& context)
    : async_echo_server<asio_ssl_server_stream>(context) {
  }

  void do_read() final {
  }
};

TEST_CASE("moved stream") {
  boost::asio::io_context ioc;

  wintls_server_context server_ctx;
  boost::wintls::stream<test_stream> moved_server_stream(ioc, server_ctx);
  boost::wintls::stream<test_stream> server_stream(std::move(moved_server_stream));

  wintls_client_context client_ctx;
  boost::wintls::stream<test_stream> moved_client_stream(ioc, client_ctx);
  boost::wintls::stream<test_stream> client_stream(std::move(moved_client_stream));

  client_stream.next_layer().connect(server_stream.next_layer());

  boost::system::error_code client_ec{};
  boost::system::error_code server_ec{};

  server_stream.async_handshake(boost::wintls::handshake_type::server,
                                [&server_ec, &server_stream](const boost::system::error_code& ec) {
                                  server_ec = ec;
                                  if (ec) {
                                    server_stream.next_layer().close();
                                  }
                                });

  client_stream.async_handshake(boost::wintls::handshake_type::client,
                                [&client_ec, &client_stream](const boost::system::error_code& ec) {
                                  client_ec = ec;
                                  if (ec) {
                                    client_stream.next_layer().close();
                                  }
                                });
  ioc.run();
  CHECK_FALSE(client_ec);
  CHECK_FALSE(server_ec);
}

TEST_CASE("handshake not done") {
  boost::wintls::context ctx{boost::wintls::method::system_default};
  boost::asio::io_context ioc;
  std::array<char, 4> buf{};

  boost::wintls::stream<boost::asio::ip::tcp::socket> stream(ioc, ctx);
  boost::system::error_code ec{};

  SECTION("write fails") {
    boost::wintls::net::write(stream, boost::wintls::net::buffer(buf), ec);
    CHECK(ec);
  }

  SECTION("async_write fails") {
    boost::wintls::net::async_write(stream, boost::wintls::net::buffer(buf),
                                    [&ec](const boost::system::error_code& error, std::size_t) {
                                      ec = error;
                                    });
    ioc.run_one();
    CHECK(ec);
  }

  SECTION("read fails") {
    boost::wintls::net::read(stream, boost::wintls::net::buffer(buf), ec);
    CHECK(ec);
  }

  SECTION("async_read fails") {
    boost::wintls::net::async_read(stream, boost::wintls::net::buffer(buf),
                                   [&ec](const boost::system::error_code& error, std::size_t) {
                                      ec = error;
                                    });
    ioc.run_one();
    CHECK(ec);
  }
}

TEST_CASE("underlying stream errors") {
  SECTION("sync test") {
    net::io_context io_context;
    echo_server<asio_ssl_server_stream> server(io_context);
    boost::system::error_code client_ec{};

    SECTION("handshake error") {
      boost::beast::test::fail_count fc(4);
      wintls_client_stream client(io_context, fc);

      client.stream.next_layer().connect(server.stream.next_layer());

      auto handshake_result = server.handshake();
      client.stream.handshake(wintls_client_stream::handshake_type::client, client_ec);
      REQUIRE_FALSE(handshake_result.get());
      CHECK(client_ec.value() == 1);
    }

    SECTION("failing read/write") {
      boost::beast::test::fail_count fc(5);
      wintls_client_stream client(io_context, fc);

      client.stream.next_layer().connect(server.stream.next_layer());

      auto handshake_result = server.handshake();
      client.stream.handshake(wintls_client_stream::handshake_type::client, client_ec);
      REQUIRE_FALSE(handshake_result.get());
      REQUIRE_FALSE(client_ec);

      SECTION("read error") {
        net::streambuf client_data;
        net::read(client.stream, client_data, client_ec);
        CHECK(client_ec.value() == 1);
      }

      SECTION("write error") {
        std::string str{"abcd"};
        net::write(client.stream, net::buffer(str), client_ec);
        CHECK(client_ec.value() == 1);
      }
    }
  }

  SECTION("async test") {
    net::io_context io_context;
    boost::system::error_code client_ec{};

    test_server server(io_context);

    SECTION("handshake error") {
      boost::beast::test::fail_count fc(4);
      wintls_client_stream client(io_context, fc);
      client.stream.next_layer().connect(server.stream.next_layer());
      server.run();
      client.stream.async_handshake(wintls_client_stream::handshake_type::client,
                                    [&client_ec](const boost::system::error_code& ec) {
                                      client_ec = ec;
                                    });
      io_context.run();
      CHECK(client_ec.value() == 1);
    }

    SECTION("failing read/write") {
      boost::beast::test::fail_count fc(5);
      wintls_client_stream client(io_context, fc);
      client.stream.next_layer().connect(server.stream.next_layer());
      boost::asio::streambuf buffer;
      server.run();

      SECTION("read error") {
        client.stream.async_handshake(wintls_client_stream::handshake_type::client,
                                      [&client_ec, &client, &buffer](const boost::system::error_code& ec) {
                                        REQUIRE_FALSE(ec);
                                        net::async_read(client.stream, buffer, [&client_ec](const boost::system::error_code& error, std::size_t) {
                                          client_ec = error;
                                        });
                                      });
        io_context.run();
        CHECK(client_ec.value() == 1);
      }

      SECTION("write error") {
        client.stream.async_handshake(wintls_client_stream::handshake_type::client,
                                      [&client_ec, &client, &buffer](const boost::system::error_code& ec) {
                                        REQUIRE_FALSE(ec);
                                        net::async_write(client.stream, buffer, [&client_ec](const boost::system::error_code& error, std::size_t) {
                                          client_ec = error;
                                        });
                                      });
        io_context.run();
        CHECK(client_ec.value() == 1);
      }
    }
  }
}

TEST_CASE("small reads") {
  using namespace std::string_literals;

  net::io_context io_context;
  const auto test_data = "Der er et yndigt land\0"s;

  SECTION("async client test") {
    async_echo_server<asio_ssl_server_stream> server(io_context);
    async_echo_client<wintls_client_stream> client(io_context, test_data);
    client.stream.next_layer().read_size(0x20);
    client.stream.next_layer().connect(server.stream.next_layer());
    server.run();
    client.run();
    io_context.run();
    CHECK(client.received_message() == test_data);
  }

  SECTION("async server test") {
    async_echo_server<wintls_server_stream> server(io_context);
    async_echo_client<wintls_client_stream> client(io_context, test_data);
    server.stream.next_layer().read_size(0x20);
    client.stream.next_layer().connect(server.stream.next_layer());
    server.run();
    client.run();
    io_context.run();
    CHECK(client.received_message() == test_data);
  }

  SECTION("sync test") {
    echo_server<asio_ssl_server_stream> server(io_context);
    echo_client<wintls_client_stream> client(io_context);
    client.stream.next_layer().read_size(0x20);
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

    CHECK(client.data<std::string>() == test_data);
  }
}
