//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_CONTEXT_FLAGS_HPP
#define BOOST_WINTLS_DETAIL_CONTEXT_FLAGS_HPP

namespace boost {
namespace wintls {
namespace detail {

constexpr DWORD client_context_flags =
  ISC_REQ_SEQUENCE_DETECT | // Detect messages received out of sequence
  ISC_REQ_REPLAY_DETECT | // Detect replayed messages
  ISC_REQ_CONFIDENTIALITY | // Encrypt messages
  ISC_RET_EXTENDED_ERROR | // When errors occur, the remote party will be notified
  ISC_REQ_ALLOCATE_MEMORY | // Allocate buffers. Free them with FreeContextBuffer
  ISC_REQ_STREAM | // Support a stream-oriented connection
  ISC_REQ_USE_SUPPLIED_CREDS | // Do not auto send client cert.
  ISC_REQ_MANUAL_CRED_VALIDATION; // Manually authenticate

constexpr DWORD server_context_flags =
  ASC_REQ_SEQUENCE_DETECT | // Detect messages received out of sequence
  ASC_REQ_REPLAY_DETECT | // Detect replayed messages
  ASC_REQ_CONFIDENTIALITY | // Encrypt messages
  ASC_RET_EXTENDED_ERROR | // When errors occur, the remote party will be notified
  ASC_REQ_ALLOCATE_MEMORY | // Allocate buffers. Free them with FreeContextBuffer
  ASC_REQ_STREAM; // Support a stream-oriented connection

} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_CONTEXT_FLAGS_HPP
