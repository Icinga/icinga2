Functions
=========

x509_to_cert_context
--------------------
.. doxygenfunction:: boost::wintls::x509_to_cert_context(const net::const_buffer &x509, file_format format)
.. doxygenfunction:: boost::wintls::x509_to_cert_context(const net::const_buffer &x509, file_format format, boost::system::error_code& ec)

import_private_key
------------------
.. doxygenfunction:: boost::wintls::import_private_key(const net::const_buffer& private_key, file_format format, const std::string& name)
.. doxygenfunction:: boost::wintls::import_private_key(const net::const_buffer& private_key, file_format format, const std::string& name, boost::system::error_code& ec)

delete_private_key
------------------
.. doxygenfunction:: delete_private_key(const std::string& name)
.. doxygenfunction:: delete_private_key(const std::string& name, boost::system::error_code& ec)

assign_private_key
------------------
.. doxygenfunction:: assign_private_key(const CERT_CONTEXT* cert, const std::string& name)
.. doxygenfunction:: assign_private_key(const CERT_CONTEXT* cert, const std::string& name, boost::system::error_code& ec)
.. _CERT_CONTEXT: https://docs.microsoft.com/en-us/windows/win32/api/wincrypt/ns-wincrypt-cert_context
