.. figure:: logo.jpg
   :alt: Boost.Wintls logo

Overview
========

This library implements TLS stream functionality for `boost::asio`_
using native Windows `SSPI/Schannel`_ implementation.

To the extend possible it provides the same functionality as the
`OpenSSL`_ implementation used by `boost::asio`_ and provides some
helper :ref:`functions<functions>` for converting and managing certificates and keys in
standard formats used by `OpenSSL`_.

Released under the `Boost Software License`_. Source code available on `GitHub`_.

Motivation
----------

`boost::asio`_ uses `OpenSSL`_ for TLS encryption which has a few
downsides when used on Windows:

* Requires maintaining a separate copy of trusted certificate
  authorities although the operating system already ships with and
  maintains a store of trusted certificates.

* When used as a server, `OpenSSL`_ requires access to the private key
  as a file readable by the running process which could potentially
  lead to security issues if measures are not taken to ensure the
  private key is properly protected.

* Installing third party libraries and software in general on Windows
  is often a complicated process since no central packaging system
  exists, so any security updates to `OpenSSL`_ would have to be
  maintained by the software using the `boost::asio`_ library.

This library avoids these issues by using the native Windows TLS
implementation (`SSPI/SChannel`_) which uses the methods for storing
certificates and keys provided by the Windows operating system itself.

Contents
--------
.. toctree::
   :maxdepth: 3

   usage
   examples
   API


.. _SSPI/Schannel: https://docs.microsoft.com/en-us/windows-server/security/tls/tls-ssl-schannel-ssp-overview/
.. _OpenSSL: https://www.openssl.org/
.. _boost::asio: https://www.boost.org/doc/libs/release/doc/html/boost_asio.html
.. _Boost Software License: https://www.boost.org/LICENSE_1_0.txt
.. _GitHub: https://github.com/laudrup/boost-wintls
