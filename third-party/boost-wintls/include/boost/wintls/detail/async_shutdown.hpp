//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_ASYNC_SHUTDOWN_HPP
#define BOOST_WINTLS_DETAIL_ASYNC_SHUTDOWN_HPP

#include <boost/wintls/detail/sspi_shutdown.hpp>

#include <boost/asio/coroutine.hpp>

namespace boost {
namespace wintls {
namespace detail {

template <typename NextLayer>
struct async_shutdown : boost::asio::coroutine {
  async_shutdown(NextLayer& next_layer, detail::sspi_shutdown& shutdown)
    : next_layer_(next_layer)
    , shutdown_(shutdown)
    , entry_count_(0) {
  }

  template <typename Self>
  void operator()(Self& self, boost::system::error_code ec = {}, std::size_t size_written = 0) {
    if (ec) {
      self.complete(ec);
      return;
    }

    ++entry_count_;
    auto is_continuation = [this] {
      return entry_count_ > 1;
    };

    ec = shutdown_();

    BOOST_ASIO_CORO_REENTER(*this) {
      if (!ec) {
        BOOST_ASIO_CORO_YIELD {
          net::async_write(next_layer_, shutdown_.buffer(), std::move(self));
        }
        shutdown_.size_written(size_written);
        self.complete({});
        return;
      } else {
        if (!is_continuation()) {
          BOOST_ASIO_CORO_YIELD {
            auto e = self.get_executor();
            net::post(e, [self = std::move(self), ec, size_written]() mutable { self(ec, size_written); });
          }
        }
        self.complete(ec);
        return;
      }
    }
  }

private:
  NextLayer& next_layer_;
  detail::sspi_shutdown& shutdown_;
  int entry_count_;
};

} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_ASYNC_SHUTDOWN_HPP
