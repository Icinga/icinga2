//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_METHOD_HPP
#define BOOST_WINTLS_METHOD_HPP

#include <boost/wintls/detail/sspi_types.hpp>

#ifndef SP_PROT_TLS1_1_SERVER
#define SP_PROT_TLS1_1_SERVER 0x100
#endif

#ifndef SP_PROT_TLS1_1_CLIENT
#define SP_PROT_TLS1_1_CLIENT 0x200
#endif

#ifndef SP_PROT_TLS1_2_SERVER
#define SP_PROT_TLS1_2_SERVER 0x400
#endif

#ifndef SP_PROT_TLS1_2_CLIENT
#define SP_PROT_TLS1_2_CLIENT 0x800
#endif

#ifndef SP_PROT_TLS1_3_SERVER
#define SP_PROT_TLS1_3_SERVER 0x1000
#endif

#ifndef SP_PROT_TLS1_3_CLIENT
#define SP_PROT_TLS1_3_CLIENT 0x2000
#endif

namespace boost {
namespace wintls {

/// Different methods supported by a context.
enum class method {
  /// Operating system defaults.
  system_default = 0,

  /// Generic SSL version 3.
  sslv3 = SP_PROT_SSL3_SERVER | SP_PROT_SSL3_CLIENT,

  /// SSL version 3 client.
  sslv3_client = SP_PROT_SSL3_CLIENT,

  /// SSL version 3 server.
  sslv3_server = SP_PROT_SSL3_SERVER,

  /// Generic TLS version 1.
  tlsv1 = SP_PROT_TLS1_SERVER | SP_PROT_TLS1_CLIENT,

  /// TLS version 1 client.
  tlsv1_client = SP_PROT_TLS1_CLIENT,

  /// TLS version 1 server.
  tlsv1_server = SP_PROT_TLS1_SERVER,

  /// Generic TLS version 1.1.
  tlsv11 = SP_PROT_TLS1_1_SERVER | SP_PROT_TLS1_1_CLIENT,

  /// TLS version 1.1 client.
  tlsv11_client = SP_PROT_TLS1_1_CLIENT,

  /// TLS version 1.1 server.
  tlsv11_server = SP_PROT_TLS1_1_SERVER,

  /// Generic TLS version 1.2.
  tlsv12 = SP_PROT_TLS1_2_SERVER | SP_PROT_TLS1_2_CLIENT,

  /// TLS version 1.2 client.
  tlsv12_client = SP_PROT_TLS1_2_CLIENT,

  /// TLS version 1.2 server.
  tlsv12_server = SP_PROT_TLS1_2_SERVER,

  /// Generic TLS version 1.3.
  tlsv13 = SP_PROT_TLS1_3_SERVER | SP_PROT_TLS1_3_CLIENT,

  /// TLS version 1.3 client.
  tlsv13_client = SP_PROT_TLS1_3_CLIENT,

  /// TLS version 1.3 server.
  tlsv13_server = SP_PROT_TLS1_3_SERVER
};

} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_METHOD_HPP
