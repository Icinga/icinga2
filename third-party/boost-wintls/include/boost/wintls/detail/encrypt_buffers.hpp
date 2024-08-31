//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_ENCRYPT_BUFFERS_HPP
#define BOOST_WINTLS_DETAIL_ENCRYPT_BUFFERS_HPP

#include <boost/wintls/detail/sspi_buffer_sequence.hpp>
#include <boost/wintls/detail/sspi_functions.hpp>
#include <boost/wintls/detail/config.hpp>

namespace boost {
namespace wintls {
namespace detail {

class encrypt_buffers : public sspi_buffer_sequence<4> {
public:
  encrypt_buffers(ctxt_handle& ctxt_handle)
    : sspi_buffer_sequence(std::array<sspi_buffer, 4> {
        SECBUFFER_STREAM_HEADER,
        SECBUFFER_DATA,
        SECBUFFER_STREAM_TRAILER,
        SECBUFFER_EMPTY
      })
    , ctxt_handle_(ctxt_handle) {
  }

  template <typename ConstBufferSequence> std::size_t operator()(const ConstBufferSequence& buffers, SECURITY_STATUS& sc) {
    if (data_.empty()) {
      sc = sspi_functions::QueryContextAttributes(ctxt_handle_.get(), SECPKG_ATTR_STREAM_SIZES, &stream_sizes_);
      if (sc != SEC_E_OK) {
        return 0;
      }
      data_.resize(stream_sizes_.cbHeader + stream_sizes_.cbMaximumMessage + stream_sizes_.cbTrailer);
    }

    const auto size_consumed = std::min(net::buffer_size(buffers), static_cast<size_t>(stream_sizes_.cbMaximumMessage));

    buffers_[0].pvBuffer = data_.data();
    buffers_[0].cbBuffer = stream_sizes_.cbHeader;

    net::buffer_copy(net::buffer(data_.data() + stream_sizes_.cbHeader, size_consumed), buffers);
    buffers_[1].pvBuffer = data_.data() + stream_sizes_.cbHeader;
    buffers_[1].cbBuffer = static_cast<ULONG>(size_consumed);

    buffers_[2].pvBuffer = data_.data() + stream_sizes_.cbHeader + size_consumed;
    buffers_[2].cbBuffer = stream_sizes_.cbTrailer;

    return size_consumed;
  }

private:
  ctxt_handle& ctxt_handle_;
  std::vector<char> data_;
  SecPkgContext_StreamSizes stream_sizes_{0, 0, 0, 0, 0};
};

} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_ENCRYPT_BUFFERS_HPP
