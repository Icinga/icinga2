//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "async_echo_client.hpp"
#include "async_echo_server.hpp"
#include "certificate.hpp"
#include "tls_record.hpp"
#include "unittest.hpp"

#include <boost/wintls.hpp>
#include "asio_ssl_server_stream.hpp"
#include "asio_ssl_client_stream.hpp"
#include "wintls_client_stream.hpp"
#include "wintls_server_stream.hpp"

#include <boost/system/error_code.hpp>
#include <boost/asio/ssl.hpp>

namespace boost {
namespace wintls {

std::ostream& operator<<(std::ostream& os, const method meth) {
  switch (meth) {
    case method::system_default:
      return os << "system_default";
    case method::sslv3:
      return os << "sslv3";
    case method::sslv3_client:
      return os << "sslv3_client";
    case method::sslv3_server:
      return os << "sslv3_server";
    case method::tlsv1:
      return os << "tlsv1";
    case method::tlsv1_client:
      return os << "tlsv1_client";
    case method::tlsv1_server:
      return os << "tlsv1_server";
    case method::tlsv11:
      return os << "tlsv11";
    case method::tlsv11_client:
      return os << "tlsv11_client";
    case method::tlsv11_server:
      return os << "tlsv11_server";
    case method::tlsv12:
      return os << "tlsv12";
    case method::tlsv12_client:
      return os << "tlsv12_client";
    case method::tlsv12_server:
      return os << "tlsv12_server";
    case method::tlsv13:
      return os << "tlsv13";
    case method::tlsv13_client:
      return os << "tlsv13_client";
    case method::tlsv13_server:
      return os << "tlsv13_server";
  }
  BOOST_UNREACHABLE_RETURN(0);
}

} // namespace wintls
} // namespace boost

namespace {

std::string wchar_to_string(const wchar_t* input) {
  const auto length = static_cast<int>(std::wcslen(input));

  const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, input, length, nullptr, 0, nullptr, nullptr);
  if (size_needed == 0) {
    boost::wintls::detail::throw_last_error("WideCharToMultiByte");
  }

  std::string output(size_needed, '\0');
  const auto size_written = WideCharToMultiByte(CP_UTF8, 0, input, length, &output[0], size_needed, nullptr, nullptr);
  if (size_written == 0) {
    boost::wintls::detail::throw_last_error("WideCharToMultiByte");
  }
  return output;
}

std::vector<BYTE> string_to_x509_name(const std::string& str) {
  DWORD size = 0;
  std::vector<BYTE> ret;
  if (!CertStrToName(X509_ASN_ENCODING, str.c_str(), CERT_X500_NAME_STR, nullptr, nullptr, &size, nullptr)) {
    boost::wintls::detail::throw_last_error("CertStrToName");
  }
  ret.resize(size);
  if (!CertStrToName(X509_ASN_ENCODING, str.c_str(), CERT_X500_NAME_STR, nullptr, ret.data(), &size, nullptr)) {
    boost::wintls::detail::throw_last_error("CertStrToName");
  }
  return ret;
}

struct cert_name_blob {
  cert_name_blob(const std::string& str)
    : data_(string_to_x509_name(str)) {
    blob.pbData = data_.data();
    blob.cbData = static_cast<DWORD>(data_.size());
  }

  CERT_NAME_BLOB blob;
private:
  std::vector<BYTE> data_;
};

boost::wintls::cert_context_ptr create_self_signed_cert(const std::string& subject) {
  cert_name_blob cert_subject(subject);
  SYSTEMTIME expiry_date;
  GetSystemTime(&expiry_date);
  expiry_date.wYear += 1;

  auto cert = CertCreateSelfSignCertificate(0,
                                            &cert_subject.blob,
                                            0,
                                            0,
                                            nullptr,
                                            0,
                                            &expiry_date,
                                            0);
  if (!cert) {
    boost::wintls::detail::throw_last_error("CertCreateSelfSignCertificate");
  }
  return boost::wintls::cert_context_ptr{cert};
}

std::string cert_container_name(const CERT_CONTEXT* cert) {
  DWORD size = 0;
  if (!CertGetCertificateContextProperty(cert,
                                         CERT_KEY_PROV_INFO_PROP_ID,
                                         nullptr,
                                         &size)) {
    boost::wintls::detail::throw_last_error("CertGetCertificateContextProperty");
  }

  std::vector<BYTE> data(size);
  if (!CertGetCertificateContextProperty(cert,
                                         CERT_KEY_PROV_INFO_PROP_ID,
                                         data.data(),
                                         &size)) {
    boost::wintls::detail::throw_last_error("CertGetCertificateContextProperty");
  }
  const auto info = reinterpret_cast<CRYPT_KEY_PROV_INFO*>(data.data());
  return wchar_to_string(info->pwszContainerName);
}
} // namespace

TEST_CASE("certificates") {
  using namespace std::string_literals;
  using namespace boost::system;

  net::io_context io_context;
  boost::wintls::context client_ctx(boost::wintls::method::system_default);
  boost::wintls::stream<test_stream> client_stream(io_context, client_ctx);

  SECTION("invalid certificate data") {
    // TODO: Instead of returning an error when given a null pointer
    // or other easily detectable invalid input, the Windows crypto
    // libraries cause the Windows equivalent of a segfault. This is
    // pretty consistent with the rest of the Windows API though.
    //
    // Figure out a way to generate invalid data that doesn't make the
    // test crash.
    /*
    using namespace boost::system;

    auto error = errc::make_error_code(errc::not_supported);

    CERT_INFO cert_info{};
    const CERT_CONTEXT bad_cert{
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
      nullptr,
      0,
      &cert_info,
      0};
    client_ctx.add_certificate_authority(&bad_cert, error);

    CHECK(error.category() == boost::system::system_category());
    CHECK(error.value() == CRYPT_E_ASN1_EOD);
    */
  }

  SECTION("server cert without private key") {
    boost::wintls::context server_ctx(boost::wintls::method::system_default);
    auto cert = x509_to_cert_context(net::buffer(test_certificate), boost::wintls::file_format::pem);

    CHECK_THROWS_WITH(server_ctx.use_certificate(cert.get()),
                      Catch::Matchers::Contains("Cannot find the certificate and private key for decryption"));

    boost::system::error_code ec{};
    server_ctx.use_certificate(cert.get(), ec);
    CHECK(ec.category() == system_category());
    CHECK(ec.value() & NTE_BAD_SIGNATURE);
  }

  SECTION("wintl server") {
    boost::wintls::context server_ctx(boost::wintls::method::system_default);
    const auto cert = create_self_signed_cert("CN=WinTLS, T=Test");
    server_ctx.use_certificate(cert.get());
    boost::wintls::stream<test_stream> server_stream(io_context, server_ctx);

    client_stream.next_layer().connect(server_stream.next_layer());

    SECTION("no certificate validation") {
      auto client_error = errc::make_error_code(errc::not_supported);
      client_stream.async_handshake(boost::wintls::handshake_type::client,
                                    [&client_error, &io_context](const boost::system::error_code& ec) {
                                      client_error = ec;
                                      io_context.stop();
                                    });

      auto server_error = errc::make_error_code(errc::not_supported);
      server_stream.async_handshake(boost::wintls::handshake_type::server,
                                    [&server_error](const boost::system::error_code& ec) {
                                      server_error = ec;
                                    });
      io_context.run();
      CHECK_FALSE(client_error);
      CHECK_FALSE(server_error);
    }

    SECTION("no trusted certificate") {
      client_ctx.verify_server_certificate(true);

      auto client_error = errc::make_error_code(errc::not_supported);
      client_stream.async_handshake(boost::wintls::handshake_type::client,
                                    [&client_error](const boost::system::error_code& ec) {
                                      client_error = ec;
                                    });

      auto server_error = errc::make_error_code(errc::not_supported);
      server_stream.async_handshake(boost::wintls::handshake_type::server,
                                    [&server_error](const boost::system::error_code& ec) {
                                      server_error = ec;
                                    });

      io_context.run();
      CHECK(client_error.category() == system_category());
      CHECK(client_error.value() == CERT_E_UNTRUSTEDROOT);
      CHECK_FALSE(server_error);
    }

    SECTION("trusted certificate verified") {
      client_ctx.verify_server_certificate(true);
      client_ctx.add_certificate_authority(cert.get());

      auto client_error = errc::make_error_code(errc::not_supported);
      client_stream.async_handshake(boost::wintls::handshake_type::client,
                                    [&client_error, &io_context](const boost::system::error_code& ec) {
                                      client_error = ec;
                                      io_context.stop();
                                    });

      auto server_error = errc::make_error_code(errc::not_supported);
      server_stream.async_handshake(boost::wintls::handshake_type::server,
                                    [&server_error](const boost::system::error_code& ec) {
                                      server_error = ec;
                                    });
      io_context.run();
      CHECK_FALSE(client_error);
      CHECK_FALSE(server_error);
    }

    boost::wintls::delete_private_key(cert_container_name(cert.get()));
  }

  SECTION("asio::ssl server") {
    boost::asio::ssl::context server_ctx(boost::asio::ssl::context::tls_server);
    server_ctx.use_certificate_chain(net::buffer(test_certificate));
    server_ctx.use_private_key(net::buffer(test_key), boost::asio::ssl::context::pem);

    boost::asio::ssl::stream<test_stream> server_stream(io_context, server_ctx);

    client_stream.next_layer().connect(server_stream.next_layer());

    SECTION("no certificate validation") {
      auto client_error = errc::make_error_code(errc::not_supported);
      client_stream.async_handshake(boost::wintls::handshake_type::client,
                                    [&client_error, &io_context](const boost::system::error_code& ec) {
                                      client_error = ec;
                                      io_context.stop();
                                    });

      auto server_error = errc::make_error_code(errc::not_supported);
      server_stream.async_handshake(asio_ssl::stream_base::server,
                                    [&server_error](const boost::system::error_code& ec) {
                                      server_error = ec;
                                    });
      io_context.run();
      CHECK_FALSE(client_error);
      CHECK_FALSE(server_error);
    }

    SECTION("no trusted certificate") {
      client_ctx.verify_server_certificate(true);

      auto client_error = errc::make_error_code(errc::not_supported);
      client_stream.async_handshake(boost::wintls::handshake_type::client,
                                    [&client_error](const boost::system::error_code& ec) {
                                      client_error = ec;
                                    });

      auto server_error = errc::make_error_code(errc::not_supported);
      server_stream.async_handshake(asio_ssl::stream_base::server,
                                    [&server_error](const boost::system::error_code& ec) {
                                      server_error = ec;
                                    });

      io_context.run();
      CHECK(client_error.category() == system_category());
      CHECK(client_error.value() == CERT_E_UNTRUSTEDROOT);
      CHECK_FALSE(server_error);
    }

    SECTION("trusted certificate verified") {
      client_ctx.verify_server_certificate(true);

      const auto cert_ptr = x509_to_cert_context(net::buffer(test_certificate), boost::wintls::file_format::pem);
      client_ctx.add_certificate_authority(cert_ptr.get());

      auto client_error = errc::make_error_code(errc::not_supported);
      client_stream.async_handshake(boost::wintls::handshake_type::client,
                                    [&client_error, &io_context](const boost::system::error_code& ec) {
                                      client_error = ec;
                                      io_context.stop();
                                    });

      auto server_error = errc::make_error_code(errc::not_supported);
      server_stream.async_handshake(asio_ssl::stream_base::server,
                                    [&server_error](const boost::system::error_code& ec) {
                                      server_error = ec;
                                    });
      io_context.run();
      CHECK_FALSE(client_error);
      CHECK_FALSE(server_error);
    }
  }
}

TEST_CASE("client certificates") {
  using namespace std::string_literals;

  SECTION("wintls client certificate missing with openssl server") {
    using namespace boost::system;
    wintls_client_context client_ctx;
    asio_ssl_server_context server_ctx;
    server_ctx.enable_client_verify();

    net::io_context io_context;
    boost::wintls::stream<test_stream> client_stream(io_context, client_ctx);
    boost::asio::ssl::stream<test_stream> server_stream(io_context, server_ctx);

    client_stream.next_layer().connect(server_stream.next_layer());

    auto client_error = errc::make_error_code(errc::not_supported);
    client_stream.async_handshake(boost::wintls::handshake_type::client,
                                  [&client_error](const boost::system::error_code& ec) {
                                    client_error = ec;
                                  });

    auto server_error = errc::make_error_code(errc::not_supported);
    server_stream.async_handshake(asio_ssl::stream_base::server,
                                  [&server_error](const boost::system::error_code& ec) {
                                    server_error = ec;
                                  });
    io_context.run();
    // client handshake is failed by server
    CHECK(client_error);
    // Note: The server error code is 0xa0000c7 or 0xc0c7 depends on the int size
    // and expected error code is 199. Error message is correct.
    // Seems like the error code lower bits are right, take the lower 2 bytes of the int.
    // It is unclear why this happens.
    CHECK_THAT(server_error.message(), Catch::Contains("peer did not return a certificate"));
    CHECK((server_error.value() & 0xff) == SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE);
  }

  SECTION("trusted wintls client certificate verified on openssl server") {
    using namespace boost::system;

    wintls_client_context client_ctx;
    client_ctx.with_test_client_cert(); // Note that if client cert is supplied, sspi will verify server cert with it.
    client_ctx.verify_server_certificate(true);

    asio_ssl_server_context server_ctx;
    server_ctx.enable_client_verify();

    net::io_context io_context;
    boost::wintls::stream<test_stream> client_stream(io_context, client_ctx);
    boost::asio::ssl::stream<test_stream> server_stream(io_context, server_ctx);

    client_stream.next_layer().connect(server_stream.next_layer());

    auto client_error = errc::make_error_code(errc::not_supported);
    client_stream.async_handshake(boost::wintls::handshake_type::client,
                                  [&client_error](const boost::system::error_code& ec) {
                                    client_error = ec;
                                  });

    auto server_error = errc::make_error_code(errc::not_supported);
    server_stream.async_handshake(asio_ssl::stream_base::server,
                                  [&server_error](const boost::system::error_code& ec) {
                                    server_error = ec;
                                  });
    io_context.run();
    CHECK_FALSE(client_error);
    CHECK_FALSE(server_error);
  }

  SECTION("trusted openssl client certificate verified on openssl server") {
    using namespace boost::system;
    asio_ssl_client_context client_ctx;
    client_ctx.with_test_client_cert();
    client_ctx.enable_server_verify();

    asio_ssl_server_context server_ctx;
    server_ctx.enable_client_verify();

    net::io_context io_context;
    boost::asio::ssl::stream<test_stream> client_stream(io_context, client_ctx);
    boost::asio::ssl::stream<test_stream> server_stream(io_context, server_ctx);

    client_stream.next_layer().connect(server_stream.next_layer());

    auto client_error = errc::make_error_code(errc::not_supported);
    client_stream.async_handshake(asio_ssl::stream_base::client,
                                  [&client_error](const boost::system::error_code& ec) {
                                    client_error = ec;
                                  });

    auto server_error = errc::make_error_code(errc::not_supported);
    server_stream.async_handshake(asio_ssl::stream_base::server,
                                  [&server_error](const boost::system::error_code& ec) {
                                    server_error = ec;
                                  });
    io_context.run();
    CHECK_FALSE(client_error);
    CHECK_FALSE(server_error);
  }

  SECTION("trusted openssl client certificate verified on wintls server") {
    using namespace boost::system;
    asio_ssl_client_context client_ctx;
    client_ctx.with_test_client_cert();
    client_ctx.enable_server_verify();

    wintls_server_context server_ctx;
    server_ctx.enable_client_verify();

    net::io_context io_context;
    boost::asio::ssl::stream<test_stream> client_stream(io_context, client_ctx);
    boost::wintls::stream<test_stream> server_stream(io_context, server_ctx);

    client_stream.next_layer().connect(server_stream.next_layer());

    auto client_error = errc::make_error_code(errc::not_supported);
    client_stream.async_handshake(asio_ssl::stream_base::client,
                                  [&client_error](const boost::system::error_code& ec) {
                                    client_error = ec;
                                  });

    auto server_error = errc::make_error_code(errc::not_supported);
    server_stream.async_handshake(boost::wintls::handshake_type::server,
                                  [&server_error](const boost::system::error_code& ec) {
                                    server_error = ec;
                                  });
    io_context.run();
    CHECK_FALSE(client_error);
    CHECK_FALSE(server_error);
  }

  SECTION("openssl client missing certificate on wintls server") {
    using namespace boost::system;
    asio_ssl_client_context client_ctx;

    wintls_server_context server_ctx;
    server_ctx.enable_client_verify();

    net::io_context io_context;
    boost::asio::ssl::stream<test_stream> client_stream(io_context, client_ctx);
    boost::wintls::stream<test_stream> server_stream(io_context, server_ctx);

    client_stream.next_layer().connect(server_stream.next_layer());

    auto client_error = errc::make_error_code(errc::not_supported);
    client_stream.async_handshake(asio_ssl::stream_base::client,
                                  [&client_error](const boost::system::error_code& ec) {
                                    client_error = ec;
                                  });

    auto server_error = errc::make_error_code(errc::not_supported);
    server_stream.async_handshake(boost::wintls::handshake_type::server,
                                  [&server_error](const boost::system::error_code& ec) {
                                    server_error = ec;
                                  });
    io_context.run();
    CHECK_FALSE(client_error);
    CHECK(server_error.value() == SEC_E_NO_CREDENTIALS);
  }

  SECTION("trusted wintls client certificate verified on wintls server") {
    using namespace boost::system;
    wintls_client_context client_ctx;
    client_ctx.with_test_client_cert();
    client_ctx.enable_server_verify();

    wintls_server_context server_ctx;
    server_ctx.enable_client_verify();

    net::io_context io_context;
    boost::wintls::stream<test_stream> client_stream(io_context, client_ctx);
    boost::wintls::stream<test_stream> server_stream(io_context, server_ctx);

    client_stream.next_layer().connect(server_stream.next_layer());

    auto client_error = errc::make_error_code(errc::not_supported);
    client_stream.async_handshake(boost::wintls::handshake_type::client,
                                  [&client_error](const boost::system::error_code& ec) {
                                    client_error = ec;
                                  });

    auto server_error = errc::make_error_code(errc::not_supported);
    server_stream.async_handshake(boost::wintls::handshake_type::server,
                                  [&server_error](const boost::system::error_code& ec) {
                                    server_error = ec;
                                  });
    io_context.run();
    CHECK_FALSE(client_error);
    CHECK_FALSE(server_error);
  }
}

TEST_CASE("failing handshakes") {
  boost::wintls::context client_ctx(boost::wintls::method::system_default);
  net::io_context io_context;
  boost::wintls::stream<test_stream> client_stream(io_context, client_ctx);
  test_stream server_stream(io_context);

  client_stream.next_layer().connect(server_stream);

  SECTION("invalid server reply") {
    using namespace boost::system;

    auto error = errc::make_error_code(errc::not_supported);
    client_stream.async_handshake(boost::wintls::handshake_type::client,
                                  [&error](const boost::system::error_code& ec) {
                                    error = ec;
                                  });

    std::array<char, 1024> buffer;
    server_stream.async_read_some(net::buffer(buffer, buffer.size()),
                                  [&buffer, &server_stream](const boost::system::error_code&, std::size_t length) {
                                    tls_record rec(net::buffer(buffer, length));
                                    REQUIRE(rec.type == tls_record::record_type::handshake);
                                    auto handshake = boost::get<tls_handshake>(rec.message);
                                    REQUIRE(handshake.type == tls_handshake::handshake_type::client_hello);
                                    // Echoing the client_hello message back should cause the handshake to fail
                                    net::write(server_stream, net::buffer(buffer));
                                  });

    io_context.run();
    CHECK(error.category() == boost::system::system_category());
    CHECK(error.value() == SEC_E_ILLEGAL_MESSAGE);
  }
}

TEST_CASE("ssl/tls versions") {
  const auto value = GENERATE(values<std::pair<boost::wintls::method, tls_version>>({
        { boost::wintls::method::tlsv1, tls_version::tls_1_0 },
        { boost::wintls::method::tlsv1_client, tls_version::tls_1_0 },
        { boost::wintls::method::tlsv11, tls_version::tls_1_1 },
        { boost::wintls::method::tlsv11_client, tls_version::tls_1_1 },
        { boost::wintls::method::tlsv12, tls_version::tls_1_2 },
        { boost::wintls::method::tlsv12_client, tls_version::tls_1_2 },
        { boost::wintls::method::tlsv13, tls_version::tls_1_3 },
        { boost::wintls::method::tlsv13_client, tls_version::tls_1_3 }
      })
    );

  const auto method = value.first;
  const auto version = value.second;

  boost::wintls::context client_ctx(method);
  net::io_context io_context;
  boost::wintls::stream<test_stream> client_stream(io_context, client_ctx);
  test_stream server_stream(io_context);

  client_stream.next_layer().connect(server_stream);

  client_stream.async_handshake(boost::wintls::handshake_type::client,
                                [method, &io_context](const boost::system::error_code& ec) {
                                  if (ec.value() == SEC_E_ALGORITHM_MISMATCH) {
                                    WARN("Protocol not supported: " << method);
                                    io_context.stop();
                                    return;
                                  }
                                  REQUIRE(ec == net::error::eof);
                                });

  std::array<char, 1024> buffer;
  server_stream.async_read_some(net::buffer(buffer, buffer.size()),
                                [&buffer, &server_stream, &version](const boost::system::error_code&, std::size_t length) {
                                  tls_record rec(net::buffer(buffer, length));
                                  REQUIRE(rec.type == tls_record::record_type::handshake);
                                  CHECK(rec.version == version);
                                  server_stream.close();
                                });

    io_context.run();
}
