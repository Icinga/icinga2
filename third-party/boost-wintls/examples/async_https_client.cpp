//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/wintls.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <regex>
#include <string>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
namespace ssl = boost::wintls;    // from <boost/wintls/wintls.hpp>

using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

// Performs an HTTP GET and prints the response
class session : public std::enable_shared_from_this<session> {
  tcp::resolver resolver_;
  ssl::stream<beast::tcp_stream> stream_;
  beast::flat_buffer buffer_; // (Must persist between reads)
  http::request<http::empty_body> req_;
  http::response<http::string_body> res_;

public:
  explicit session(net::io_context& ioc, ssl::context& ctx)
    : resolver_(net::make_strand(ioc))
    , stream_(net::make_strand(ioc), ctx) {
  }

  // Start the asynchronous operation
  void run(const std::string& host, const std::string& port, const std::string& path, unsigned version) {
    // Set SNI hostname (many hosts need this to handshake successfully)
    stream_.set_server_hostname(host);

    // Enable Check whether the Server Certificate was revoked
    stream_.set_certificate_revocation_check(true);

    // Set up an HTTP GET request message
    req_.version(version);
    req_.method(http::verb::get);
    req_.target(path);
    req_.set(http::field::host, host);
    req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Look up the domain name
    resolver_.async_resolve(host, port, beast::bind_front_handler(&session::on_resolve, shared_from_this()));
  }

  void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec)
      return fail(ec, "resolve");

    // Set a timeout on the operation
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(stream_).async_connect(results, beast::bind_front_handler(&session::on_connect, shared_from_this()));
  }

  void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
    if (ec)
      return fail(ec, "connect");

    // Perform the SSL handshake
    stream_.async_handshake(boost::wintls::handshake_type::client, beast::bind_front_handler(&session::on_handshake, shared_from_this()));
  }

  void on_handshake(beast::error_code ec) {
    if (ec)
      return fail(ec, "handshake");

    // Set a timeout on the operation
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    http::async_write(stream_, req_, beast::bind_front_handler(&session::on_write, shared_from_this()));
  }

  void on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
      return fail(ec, "write");

    // Receive the HTTP response
    http::async_read(stream_, buffer_, res_, beast::bind_front_handler(&session::on_read, shared_from_this()));
  }

  void on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
      return fail(ec, "read");

    // Write the message to standard out
    std::cout << res_ << std::endl;

    // Set a timeout on the operation
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    // Gracefully close the stream
    stream_.async_shutdown(beast::bind_front_handler(&session::on_shutdown, shared_from_this()));
  }

  void on_shutdown(beast::error_code ec) {
    if (ec == net::error::eof) {
      // Rationale:
      // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
      ec = {};
    }
    if (ec)
      return fail(ec, "shutdown");

    // If we get here then the connection is closed gracefully
  }
};

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  // Exactly one command line argument required - the HTTPS URL
  if(argc != 2) {
    std::cerr << "Usage: " << argv[0] << " [HTTPS_URL]\n\n";
    std::cerr << "Example: " << argv[0] << " https://www.boost.org/LICENSE_1_0.txt\n";
    return EXIT_FAILURE;
  }

  const std::string url{argv[1]};

  // Very basic URL matching. Not a full URL validator.
  std::regex re("https://([^/$:]+):?([^/$]*)(/?.*)");
  std::smatch what;
  if(!regex_match(url, what, re)) {
    std::cerr << "Invalid or unsupported URL: " << url << "\n";
    return EXIT_FAILURE;
  }

  // Get the relevant parts of the URL
  const std::string host = std::string(what[1]);
  // Use default HTTPS port (443) if not specified
  const std::string port = what[2].length() > 0 ? what[2].str() : "443";
  // Use default path ('/') if not specified
  const std::string path = what[3].length() > 0 ? what[3].str() : "/";

  // Use HTTP/1.1
  const unsigned version = 11;

  // The io_context is required for all I/O
  net::io_context ioc;

  // The SSL context is required, and holds certificates
  ssl::context ctx{boost::wintls::method::system_default};

  // Use the operating systems default certificates for verification
  ctx.use_default_certificates(true);

  // Verify the remote server's certificate
  ctx.verify_server_certificate(true);

  // Launch the asynchronous operation
  // The session is constructed with a strand to
  // ensure that handlers do not execute concurrently.
  std::make_shared<session>(ioc, ctx)->run(host, port, path, version);

  // Run the I/O service. The call will return when
  // the get operation is complete.
  ioc.run();

  return EXIT_SUCCESS;
}
