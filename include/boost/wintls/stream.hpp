//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_STREAM_HPP
#define BOOST_WINTLS_STREAM_HPP

#include <boost/wintls/error.hpp>
#include <boost/wintls/handshake_type.hpp>

#include <boost/wintls/detail/async_handshake.hpp>
#include <boost/wintls/detail/async_read.hpp>
#include <boost/wintls/detail/async_shutdown.hpp>
#include <boost/wintls/detail/async_write.hpp>
#include <boost/wintls/detail/sspi_stream.hpp>

#include <boost/asio/compose.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>

#include <memory>

namespace boost {
namespace wintls {

/** Provides stream-oriented functionality using Windows SSPI/Schannel.
 *
 * The stream class template provides asynchronous and blocking
 * stream-oriented functionality using Windows SSPI/Schannel.
 *
 * @tparam NextLayer The type representing the next layer, to which
 * data will be read and written during operations. For synchronous
 * operations, the type must support the <em>SyncStream</em> concept.
 * For asynchronous operations, the type must support the
 * <em>AsyncStream</em> concept.
 */
template<class NextLayer>
class stream {
public:
  /// The type of the next layer.
  using next_layer_type = typename std::remove_reference<NextLayer>::type;

  /// The type of the executor associated with the object.
  using executor_type = typename std::remove_reference<next_layer_type>::type::executor_type;

  /** Construct a stream.
   *
   * This constructor creates a stream and initialises the underlying
   * stream object.
   *
   *  @param arg The argument to be passed to initialise the
   *  underlying stream.
   *  @param ctx The wintls @ref context to be used for the stream.
   */
  template <class Arg>
  stream(Arg&& arg, context& ctx)
    : next_layer_(std::forward<Arg>(arg))
    , sspi_stream_(std::make_unique<detail::sspi_stream>(ctx)) {
  }

  /** Get the executor associated with the object.
   *
   * This function may be used to obtain the executor object that the
   * stream uses to dispatch handlers for asynchronous operations.
   *
   * @return A copy of the executor that stream will use to dispatch
   * handlers.
   */
  executor_type get_executor() {
    return next_layer().get_executor();
  }

  /** Get a reference to the next layer.
   *
   * This function returns a reference to the next layer in a stack of
   * stream layers.
   *
   * @return A reference to the next layer in the stack of stream
   * layers.  Ownership is not transferred to the caller.
   */
  const next_layer_type& next_layer() const {
    return next_layer_;
  }

  /** Get a reference to the next layer.
   *
   * This function returns a reference to the next layer in a stack of
   * stream layers.
   *
   * @return A reference to the next layer in the stack of stream
   * layers.  Ownership is not transferred to the caller.
   */
  next_layer_type& next_layer() {
    return next_layer_;
  }

  /** Set SNI hostname
   *
   * Sets the SNI hostname the client will use for requesting and
   * validating the server certificate.
   *
   * Only used when handshake is performed as @ref
   * handshake_type::client
   *
   * @param hostname The hostname to use in certificate validation
   */
  void set_server_hostname(const std::string& hostname) {
    sspi_stream_->handshake.set_server_hostname(hostname);
  }

  /** Set revocation checking
   *
   *  Enable revocation checking for remote certificates.
   *
   *  Enabling this also causes the TLS certificate status request extension
   *  to be sent during the handshake. (I.e. we request OCSP stapling from the remote.)
   *
   *  @param check Whether to enable revocation checking
   */
  void set_certificate_revocation_check(bool check) {
    sspi_stream_->handshake.set_certificate_revocation_check(check);
  }

  /** Perform TLS handshaking.
   *
   * This function is used to perform TLS handshaking on the
   * stream. The function call will block until handshaking is
   * complete or an error occurs.
   *
   * @param type The @ref handshake_type to be performed, i.e. client
   * or server.
   * @param ec Set to indicate what error occurred, if any.
   */
  void handshake(handshake_type type, boost::system::error_code& ec) {
    sspi_stream_->handshake(type);

    detail::sspi_handshake::state state;
    while((state = sspi_stream_->handshake()) != detail::sspi_handshake::state::done) {
      switch (state) {
        case detail::sspi_handshake::state::data_needed: {
          std::size_t size_read = next_layer_.read_some(sspi_stream_->handshake.in_buffer(), ec);
          if (ec) {
            return;
          }
          sspi_stream_->handshake.size_read(size_read);
          continue;
        }
        case detail::sspi_handshake::state::data_available: {
          std::size_t size_written = net::write(next_layer_, sspi_stream_->handshake.out_buffer(), ec);
          if (ec) {
            return;
          }
          sspi_stream_->handshake.size_written(size_written);
          continue;
        }
        case detail::sspi_handshake::state::error:
          ec = sspi_stream_->handshake.last_error();
          return;
        case detail::sspi_handshake::state::done_with_data:{
          std::size_t size_written = net::write(next_layer_, sspi_stream_->handshake.out_buffer(), ec);
          if (ec) {
            return;
          }
          sspi_stream_->handshake.size_written(size_written);
          return;
        }
        case detail::sspi_handshake::state::error_with_data:{
          std::size_t size_written = net::write(next_layer_, sspi_stream_->handshake.out_buffer(), ec);
          if (ec) {
            return;
          }
          sspi_stream_->handshake.size_written(size_written);
          return;
        }
        case detail::sspi_handshake::state::done:
          BOOST_UNREACHABLE_RETURN(0);
      }
    }
  }

  /** Perform TLS handshaking.
   *
   * This function is used to perform TLS handshaking on the
   * stream. The function call will block until handshaking is
   * complete or an error occurs.
   *
   * @param type The @ref handshake_type to be performed, i.e. client
   * or server.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void handshake(handshake_type type) {
    boost::system::error_code ec{};
    handshake(type, ec);
    if (ec) {
      detail::throw_error(ec);
    }
  }

  /** Start an asynchronous TLS handshake.
   *
   * This function is used to asynchronously perform an TLS
   * handshake on the stream. This function call always returns
   * immediately.
   *
   * @param type The @ref handshake_type to be performed, i.e. client
   * or server.
   * @param handler The handler to be called when the operation
   * completes. The implementation takes ownership of the handler by
   * performing a decay-copy. The handler must be invocable with this
   * signature:
   * @code
   * void handler(
   *     boost::system::error_code // Result of operation.
   * );
   * @endcode
   *
   * @note Regardless of whether the asynchronous operation completes
   * immediately or not, the handler will not be invoked from within
   * this function. Invocation of the handler will be performed in a
   * manner equivalent to using `net::post`.
   */
  template <class CompletionToken>
  auto async_handshake(handshake_type type, CompletionToken&& handler) {
    return boost::asio::async_compose<CompletionToken, void(boost::system::error_code)>(
        detail::async_handshake<next_layer_type>{next_layer_, sspi_stream_->handshake, type}, handler);
  }

  /** Read some data from the stream.
   *
   * This function is used to read data from the stream. The function
   * call will block until one or more bytes of data has been read
   * successfully, or until an error occurs.
   *
   * @param ec Set to indicate what error occurred, if any.
   * @param buffers The buffers into which the data will be read.
   *
   * @returns The number of bytes read.
   *
   * @note The `read_some` operation may not read all of the requested
   * number of bytes. Consider using the `net::read` function if you
   * need to ensure that the requested amount of data is read before
   * the blocking operation completes.
   */
  template <class MutableBufferSequence>
  size_t read_some(const MutableBufferSequence& buffers, boost::system::error_code& ec) {
    detail::sspi_decrypt::state state;
    while((state = sspi_stream_->decrypt(buffers)) == detail::sspi_decrypt::state::data_needed) {
      std::size_t size_read = next_layer_.read_some(sspi_stream_->decrypt.input_buffer, ec);
      if (ec) {
        return 0;
      }
      sspi_stream_->decrypt.size_read(size_read);
      continue;
    }

    if (state == detail::sspi_decrypt::state::error) {
      ec = sspi_stream_->decrypt.last_error();
      return 0;
    }

    return sspi_stream_->decrypt.size_decrypted;
  }

  /** Read some data from the stream.
   *
   * This function is used to read data from the stream. The function
   * call will block until one or more bytes of data has been read
   * successfully, or until an error occurs.
   *
   * @param buffers The buffers into which the data will be read.
   *
   * @returns The number of bytes read.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @note The `read_some` operation may not read all of the requested
   * number of bytes. Consider using the `net::read` function if you
   * need to ensure that the requested amount of data is read before
   * the blocking operation completes.
   */
  template <class MutableBufferSequence>
  size_t read_some(const MutableBufferSequence& buffers) {
    boost::system::error_code ec{};
    auto read = read_some(buffers, ec);
    if (ec) {
      detail::throw_error(ec);
    }
    return read;
  }

  /** Start an asynchronous read.
   *
   * This function is used to asynchronously read one or more bytes of
   * data from the stream. The function call always returns
   * immediately.
   *
   * @param buffers The buffers into which the data will be
   * read. Although the buffers object may be copied as necessary,
   * ownership of the underlying buffers is retained by the caller,
   * which must guarantee that they remain valid until the handler is
   * called.
   * @param handler The handler to be called when the read operation
   * completes.  Copies will be made of the handler as required. The
   * equivalent function signature of the handler must be:
   * @code
   * void handler(
   *     const boost::system::error_code& error, // Result of operation.
   *     std::size_t bytes_transferred           // Number of bytes read.
   * ); @endcode
   *
   * @note The `async_read_some` operation may not read all of the
   * requested number of bytes. Consider using the `net::async_read`
   * function if you need to ensure that the requested amount of data
   * is read before the asynchronous operation completes.
   */
  template <class MutableBufferSequence, class CompletionToken>
  auto async_read_some(const MutableBufferSequence& buffers, CompletionToken&& handler) {
    return boost::asio::async_compose<CompletionToken, void(boost::system::error_code, std::size_t)>(
        detail::async_read<next_layer_type, MutableBufferSequence>{next_layer_, buffers, sspi_stream_->decrypt}, handler);
  }

  /** Write some data to the stream.
   *
   * This function is used to write data on the stream. The function
   * call will block until one or more bytes of data has been written
   * successfully, or until an error occurs.
   *
   * @param buffers The data to be written.
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of bytes written.
   *
   * @note The `write_some` operation may not transmit all of the data
   * to the peer. Consider using the `net::write` function if you need
   * to ensure that all data is written before the blocking operation
   * completes.
   */
  template <class ConstBufferSequence>
  std::size_t write_some(const ConstBufferSequence& buffers, boost::system::error_code& ec) {
    std::size_t bytes_consumed = sspi_stream_->encrypt(buffers, ec);
    if (ec) {
      return 0;
    }

    net::write(next_layer_, sspi_stream_->encrypt.buffers, ec);
    if (ec) {
      return 0;
    }

    return bytes_consumed;
  }

  /** Write some data to the stream.
   *
   * This function is used to write data on the stream. The function
   * call will block until one or more bytes of data has been written
   * successfully, or until an error occurs.
   *
   * @param buffers The data to be written.
   *
   * @returns The number of bytes written.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @note The `write_some` operation may not transmit all of the data
   * to the peer. Consider using the `net::write` function if you need
   * to ensure that all data is written before the blocking operation
   * completes.
   */
  template <class ConstBufferSequence>
  std::size_t write_some(const ConstBufferSequence& buffers) {
    boost::system::error_code ec{};
    auto wrote = write_some(buffers, ec);
    if (ec) {
      detail::throw_error(ec);
    }
    return wrote;
  }


  /** Start an asynchronous write.
   *
   * This function is used to asynchronously write one or more bytes
   * of data to the stream. The function call always returns
   * immediately.
   *
   * @param buffers The data to be written to the stream. Although the
   * buffers object may be copied as necessary, ownership of the
   * underlying buffers is retained by the caller, which must
   * guarantee that they remain valid until the handler is called.
   * @param handler The handler to be called when the write operation
   * completes.  Copies will be made of the handler as required. The
   * equivalent function signature of the handler must be:
   * @code
   * void handler(
   *     const boost::system::error_code& error, // Result of operation.
   *     std::size_t bytes_transferred           // Number of bytes written.
   * );
   * @endcode
   *
   * @note The `async_write_some` operation may not transmit all of
   * the data to the peer. Consider using the `net::async_write`
   * function if you need to ensure that all data is written before
   * the asynchronous operation completes.
   */
  template <class ConstBufferSequence, class CompletionToken>
  auto async_write_some(const ConstBufferSequence& buffers, CompletionToken&& handler) {
    return boost::asio::async_compose<CompletionToken, void(boost::system::error_code, std::size_t)>(
        detail::async_write<next_layer_type, ConstBufferSequence>{next_layer_, buffers, sspi_stream_->encrypt}, handler);
  }

  /** Shut down TLS on the stream.
   *
   * This function is used to shut down TLS on the stream. The
   * function call will block until TLS has been shut down or an
   * error occurs.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  void shutdown(boost::system::error_code& ec) {
    ec = sspi_stream_->shutdown();
    if (ec) {
      return;
    }
    std::size_t size_written = net::write(next_layer_, sspi_stream_->shutdown.buffer(), ec);
    if (!ec) {
      sspi_stream_->shutdown.size_written(size_written);
    }
  }

  /** Shut down TLS on the stream.
   *
   * This function is used to shut down TLS on the stream. The
   * function call will block until TLS has been shut down or an error
   * occurs.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void shutdown() {
    boost::system::error_code ec{};
    shutdown(ec);
    if (ec) {
      detail::throw_error(ec);
    }
  }

  /** Asynchronously shut down TLS on the stream.
   *
   * This function is used to asynchronously shut down TLS on the
   * stream. This function call always returns immediately.
   *
   * @param handler The handler to be called when the handshake
   * operation completes. Copies will be made of the handler as
   * required. The equivalent function signature of the handler must
   * be:
   * @code void handler(
   *     const boost::system::error_code& error // Result of operation.
   *);
   * @endcode
   */
  template <class CompletionToken>
  auto async_shutdown(CompletionToken&& handler) {
    return boost::asio::async_compose<CompletionToken, void(boost::system::error_code)>(
        detail::async_shutdown<next_layer_type>{next_layer_, sspi_stream_->shutdown}, handler);
  }

private:
  NextLayer next_layer_;
  std::unique_ptr<detail::sspi_stream> sspi_stream_;
};

} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_STREAM_HPP
