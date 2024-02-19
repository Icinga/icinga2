.. cpp:namespace:: boost::wintls

Usage
=====

This library implements a stream similar to
`boost::asio::ssl::stream`_. Basic knowledge of `boost::asio`_ is
expected as this documentation mainly deals with the specifics of this
library.

Setting up a context
--------------------

Client usage
~~~~~~~~~~~~

A :class:`context` is required for
holding certificates and for setting options like which TLS versions
to support.

To construct a context using the operating system default methods:
::

   #include <boost/wintls.hpp>

   boost::wintls::context ctx(boost::wintls::method::system_default);

While that is all which is required to construct a context for
client-side operations, most users would at least want to enable
certificate validation with :func:`context::verify_server_certificate`
in addition to either using the standard operating system certificates
with :func:`context::use_default_certificates` or providing
certificates with :func:`context::add_certificate_authority`.

For example, initializing a context which will use the certificates
provided by the operating system and verify the server certificate,
would look something like:
::

   #include <boost/wintls.hpp>

   boost::wintls::context ctx(boost::wintls::method::system_default);
   ctx.use_default_certificates(true);
   ctx.verify_server_certificate(true);


Server usage
~~~~~~~~~~~~

When creating a context for server usage, a certificate is required as
well as private key. Unlike `OpenSSL`_ Windows has the concept of a
certificate with an associated private key that must be available to
the process or user running the server process. Such a certificate can
be used using :func:`context::use_certificate`.

This library provides :ref:`functions<functions>` for helping interact
with standard key and certificate formats used by `OpenSSL`_ as
demonstrated in the :ref:`examples<examples>`.

Initializing a stream
---------------------

Assuming an underlying TCP stream, once the context has been setup
a :class:`stream` can be constructed like:
::

   boost::wintl::stream<ip::tcp::socket> my_stream(my_io_service, ctx);

In the case of a TCP stream, the underlying stream needs to be
connected before it can be used. To access the underlying stream use
the :func:`stream::next_layer` member function.

Handshaking
-----------

Before a TLS stream can be used, a handshake needs to be performed,
either as a server or a client. Similar to the rest of the
`boost::asio`_ functions both a synchronous and asynchronous version
exists.

When performing a handshake as a client, it is often required to
include the hostname of the server in the handshake with
:func:`stream::set_server_hostname`.

Performing a synchronous handshake as a client might look
like:
::

   strean.set_server_hostname("wintls.dev");
   stream.handshake(boost::wintls::handshake_type::client);

Similar to the `boost::asio`_ functions, this library provides
overloads for accepting a `boost::system`_::error_codes.

Using the stream
----------------

Once the stream has been constructed and a successful handshake has
been done it can be used with all the usual operations provided by the
`boost::asio`_ library.

Most users would probably not use the member functions on the stream
like :func:`stream::read_some` directly but instead use `boost::asio`_
functions like `boost::asio::write`_ or `boost::asio::async_read_until`_.

Please see the :ref:`examples<examples>` for full examples on how this
library can be used.

.. _OpenSSL: https://www.openssl.org/
.. _boost::asio: https://www.boost.org/doc/libs/release/doc/html/boost_asio.html
.. _boost::asio::ssl::stream: https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/ssl__stream.html
.. _boost::system: https://www.boost.org/doc/libs/release/libs/system/doc/html/system.html
.. _boost::asio::write: https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/write.html
.. _boost::asio::async_read_until: https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/async_read_until.html
