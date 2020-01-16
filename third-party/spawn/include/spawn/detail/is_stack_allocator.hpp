//
// detail/is_stack_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2019 Casey Bodley (cbodley at redhat dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <type_traits>

#include <boost/type_traits/make_void.hpp>

namespace spawn::detail {

template <typename T, typename = void>
struct is_stack_allocator : std::false_type {};

template <typename T>
struct is_stack_allocator<T, boost::void_t<decltype(
    // boost::context::stack_context c = salloc.allocate();
    std::declval<boost::context::stack_context>() = std::declval<T&>().allocate(),
    // salloc.deallocate(c);
    std::declval<T&>().deallocate(std::declval<boost::context::stack_context&>())
    )>> : std::true_type {};

} // namespace spawn::detail
