//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "certificate.hpp"

#include <boost/wintls.hpp>

#include <boost/asio.hpp>

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>

using boost::asio::ip::tcp;

constexpr std::size_t max_length = 1024;

class client {
public:
  client(boost::asio::io_context& io_context,
         boost::wintls::context& context,
         const tcp::resolver::results_type& endpoints)
      : stream_(io_context, context) {
    connect(endpoints);
  }

private:
  void connect(const tcp::resolver::results_type& endpoints) {
    boost::asio::async_connect(
        stream_.next_layer(), endpoints, [this](const boost::system::error_code& error,
                                                const tcp::endpoint& /*endpoint*/) {
          if (!error) {
            handshake();
          } else {
            std::cerr << "Connect failed: " << error.message() << "\n";
          }
        });
  }

  void handshake() {
    stream_.async_handshake(boost::wintls::handshake_type::client,
                            [this](const boost::system::error_code& error) {
      if (!error) {
        send_request();
      } else {
        std::cerr << "Handshake failed: " << error.message() << "\n";
      }
    });
  }

  void send_request() {
    std::cout << "Enter message: ";
    std::cin.getline(request_, max_length);
    size_t request_length = std::strlen(request_);

    boost::asio::async_write(
        stream_, boost::asio::buffer(request_, request_length),
        [this](const boost::system::error_code& error, std::size_t length) {
          if (!error) {
            receive_response(length);
          } else {
            std::cerr << "Write failed: " << error.message() << "\n";
          }
        });
  }

  void receive_response(std::size_t size) {
    boost::asio::async_read(
        stream_, boost::asio::buffer(reply_, size),
        [this](const boost::system::error_code& ec, std::size_t length) {
          if (!ec) {
            std::cout << "Reply: ";
            std::cout.write(reply_, static_cast<std::streamsize>(length));
            std::cout << "\n";
            stream_.async_shutdown([](const boost::system::error_code& error) {
              if(error) {
                std::cerr << "Shutdown failed: " << error.message() << "\n";
              }
            });
          } else {
            std::cerr << "Read failed: " << ec.message() << "\n";
          }
        });
  }

  boost::wintls::stream<tcp::socket> stream_;
  char request_[max_length];
  char reply_[max_length];
};

int main(int argc, char* argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);

    boost::wintls::context ctx(boost::wintls::method::system_default);

    // Convert X509 PEM bytes to Windows CERT_CONTEXT
    auto certificate = boost::wintls::x509_to_cert_context(boost::asio::buffer(x509_certificate),
                                                           boost::wintls::file_format::pem);

    // Add certifcate as a trusted certifcate authority and verify it on handshake
    ctx.add_certificate_authority(certificate.get());
    ctx.verify_server_certificate(true);

    client c(io_context, ctx, endpoints);

    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
