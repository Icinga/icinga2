//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_DECRYPTED_DATA_BUFFER_HPP
#define BOOST_WINTLS_DETAIL_DECRYPTED_DATA_BUFFER_HPP

#include <boost/wintls/detail/config.hpp>

#include <array>
#include <cassert>
#include <cstddef>

namespace boost {
namespace wintls {
namespace detail {

template <std::size_t BufferSize>
class decrypted_data_buffer {
public:
  std::size_t empty() const {
    return available_data_.size() == 0;
  }

  template <class MutableBufferSequence>
  std::size_t get(const MutableBufferSequence& buffer) {
    const auto size = net::buffer_copy(buffer, available_data_);
    available_data_ += size;
    return size;
  }

  template <class ConstBufferSequence>
  void fill(const ConstBufferSequence& buffer) {
    assert(available_data_.size() == 0);
    const auto size = net::buffer_copy(net::buffer(buffer_), buffer);
    available_data_ = net::buffer(buffer_.data(), size);
  }

private:
  net::mutable_buffer available_data_;
  std::array<char, BufferSize> buffer_;
};

} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_DECRYPTED_DATA_BUFFER_HPP
