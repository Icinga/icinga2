//
// spawn.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2017 Oliver Kowalke (oliver dot kowalke at gmail dot com)
// Copyright (c) 2019 Casey Bodley (cbodley at redhat dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <memory>

#include <boost/context/fixedsize_stack.hpp>
#include <boost/context/segmented_stack.hpp>
#include <boost/system/system_error.hpp>

#include <spawn/detail/net.hpp>
#include <spawn/detail/is_stack_allocator.hpp>

namespace spawn {
namespace detail {

  class continuation_context;

} // namespace detail

/// Context object represents the current execution context.
/**
 * The basic_yield_context class is used to represent the current execution
 * context. A basic_yield_context may be passed as a handler to an
 * asynchronous operation. For example:
 *
 * @code template <typename Handler>
 * void my_continuation(basic_yield_context<Handler> yield)
 * {
 *   ...
 *   std::size_t n = my_socket.async_read_some(buffer, yield);
 *   ...
 * } @endcode
 *
 * The initiating function (async_read_some in the above example) suspends the
 * current execution context, e.g. reifies a continuation. The continuation
 * is resumed when the asynchronous operation completes, and the result of
 * the operation is returned.
 */
template <typename Handler>
class basic_yield_context
{
public:
  /// Construct a yield context to represent the specified execution context.
  /**
   * Most applications do not need to use this constructor. Instead, the
   * spawn() function passes a yield context as an argument to the continuation
   * function.
   */
  basic_yield_context(
      const std::weak_ptr<detail::continuation_context>& callee,
      detail::continuation_context& caller, Handler& handler)
    : callee_(callee),
      caller_(caller),
      handler_(handler),
      ec_(0)
  {
  }

  /// Construct a yield context from another yield context type.
  /**
   * Requires that OtherHandler be convertible to Handler.
   */
  template <typename OtherHandler>
  basic_yield_context(const basic_yield_context<OtherHandler>& other)
    : callee_(other.callee_),
      caller_(other.caller_),
      handler_(other.handler_),
      ec_(other.ec_)
  {
  }

  /// Return a yield context that sets the specified error_code.
  /**
   * By default, when a yield context is used with an asynchronous operation, a
   * non-success error_code is converted to system_error and thrown. This
   * operator may be used to specify an error_code object that should instead be
   * set with the asynchronous operation's result. For example:
   *
   * @code template <typename Handler>
   * void my_continuation(basic_yield_context<Handler> yield)
   * {
   *   ...
   *   std::size_t n = my_socket.async_read_some(buffer, yield[ec]);
   *   if (ec)
   *   {
   *     // An error occurred.
   *   }
   *   ...
   * } @endcode
   */
  basic_yield_context operator[](boost::system::error_code& ec) const
  {
    basic_yield_context tmp(*this);
    tmp.ec_ = &ec;
    return tmp;
  }

#if defined(GENERATING_DOCUMENTATION)
private:
#endif // defined(GENERATING_DOCUMENTATION)
  std::weak_ptr<detail::continuation_context> callee_;
  detail::continuation_context& caller_;
  Handler handler_;
  boost::system::error_code* ec_;
};

#if defined(GENERATING_DOCUMENTATION)
/// Context object that represents the current execution context.
using yield_context = basic_yield_context<unspecified>;
#else // defined(GENERATING_DOCUMENTATION)
using yield_context = basic_yield_context<
  detail::net::executor_binder<void(*)(), detail::net::executor>>;
#endif // defined(GENERATING_DOCUMENTATION)

/**
 * @defgroup spawn spawn::spawn
 *
 * @brief Start a new execution context with a new stack.
 *
 * The spawn() function is a high-level wrapper over the Boost.Context
 * library (callcc()/continuation). This function enables programs to
 * implement asynchronous logic in a synchronous manner, as illustrated
 * by the following example:
 *
 * @code spawn::spawn(my_strand, do_echo);
 *
 * // ...
 *
 * void do_echo(spawn::yield_context yield)
 * {
 *   try
 *   {
 *     char data[128];
 *     for (;;)
 *     {
 *       std::size_t length =
 *         my_socket.async_read_some(
 *           boost::asio::buffer(data), yield);
 *
 *       boost::asio::async_write(my_socket,
 *           boost::asio::buffer(data, length), yield);
 *     }
 *   }
 *   catch (std::exception& e)
 *   {
 *     // ...
 *   }
 * } @endcode
 */
/*@{*/

/// Start a new execution context (with new stack), calling the specified handler
/// when it completes.
/**
 * This function is used to launch a new execution context on behalf of callcc()
 * and continuation.
 *
 * @param function The continuation function. The function must have the signature:
 * @code void function(basic_yield_context<Handler> yield); @endcode
 *
 * @param salloc Boost.Context uses stack allocators to create stacks.
 */
template <typename Function, typename StackAllocator = boost::context::default_stack>
auto spawn(Function&& function, StackAllocator&& salloc = StackAllocator())
  -> typename std::enable_if<detail::is_stack_allocator<
       typename std::decay<StackAllocator>::type>::value>::type;

/// Start a new execution context (with new stack), calling the specified handler
/// when it completes.
/**
 * This function is used to launch a new execution context on behalf of callcc()
 * and continuation.
 *
 * @param handler A handler to be called when the continuation exits. More
 * importantly, the handler provides an execution context (via the the handler
 * invocation hook) for the continuation. The handler must have the signature:
 * @code void handler(); @endcode
 *
 * @param function The continuation function. The function must have the signature:
 * @code void function(basic_yield_context<Handler> yield); @endcode
 *
 * @param salloc Boost.Context uses stack allocators to create stacks.
 */
template <typename Handler, typename Function,
          typename StackAllocator = boost::context::default_stack>
auto spawn(Handler&& handler, Function&& function,
           StackAllocator&& salloc = StackAllocator())
  -> typename std::enable_if<!detail::net::is_executor<typename std::decay<Handler>::type>::value &&
       !std::is_convertible<Handler&, detail::net::execution_context&>::value &&
       !detail::is_stack_allocator<typename std::decay<Function>::type>::value &&
       detail::is_stack_allocator<typename std::decay<StackAllocator>::type>::value>::type;

/// Start a new execution context (with new stack), inheriting the execution context of another.
/**
 * This function is used to launch a new execution context on behalf of callcc()
 * and continuation.
 *
 * @param ctx Identifies the current execution context as a parent of the new
 * continuation. This specifies that the new continuation should inherit the
 * execution context of the parent. For example, if the parent continuation is
 * executing in a particular strand, then the new continuation will execute in the
 * same strand.
 *
 * @param function The continuation function. The function must have the signature:
 * @code void function(basic_yield_context<Handler> yield); @endcode
 *
 * @param salloc Boost.Context uses stack allocators to create stacks.
 */
template <typename Handler, typename Function,
          typename StackAllocator = boost::context::default_stack>
auto spawn(basic_yield_context<Handler> ctx, Function&& function,
           StackAllocator&& salloc = StackAllocator())
  -> typename std::enable_if<detail::is_stack_allocator<
      typename std::decay<StackAllocator>::type>::value>::type;

/// Start a new execution context (with new stack) that executes on a given executor.
/**
 * This function is used to launch a new execution context on behalf of callcc()
 * and continuation.
 *
 * @param ex Identifies the executor that will run the continuation. The new
 * continuation is implicitly given its own strand within this executor.
 *
 * @param function The continuations function. The function must have the signature:
 * @code void function(yield_context yield); @endcode
 *
 * @param salloc Boost.Context uses stack allocators to create stacks.
 */
template <typename Function, typename Executor,
          typename StackAllocator = boost::context::default_stack>
auto spawn(const Executor& ex, Function&& function,
           StackAllocator&& salloc = StackAllocator())
  -> typename std::enable_if<detail::net::is_executor<Executor>::value &&
       detail::is_stack_allocator<typename std::decay<StackAllocator>::type>::value>::type;

/// Start a new execution context (with new stack) that executes on a given strand.
/**
 * This function is used to launch a new execution context on behalf of callcc()
 * and continuation.
 *
 * @param ex Identifies the strand that will run the continuation.
 *
 * @param function The continuation function. The function must have the signature:
 * @code void function(yield_context yield); @endcode
 *
 * @param salloc Boost.Context uses stack allocators to create stacks.
 */
template <typename Function, typename Executor,
          typename StackAllocator = boost::context::default_stack>
auto spawn(const detail::net::strand<Executor>& ex,
           Function&& function, StackAllocator&& salloc = StackAllocator())
  -> typename std::enable_if<detail::is_stack_allocator<
       typename std::decay<StackAllocator>::type>::value>::type;

/// Start a new stackful context (with new stack) that executes on a given execution context.
/**
 * This function is used to launch a new execution context on behalf of callcc()
 * and continuation.
 *
 * @param ctx Identifies the execution context that will run the continuation. The
 * new continuation is implicitly given its own strand within this execution
 * context.
 *
 * @param function The continuation function. The function must have the signature:
 * @code void function(yield_context yield); @endcode
 *
 * @param salloc Boost.Context uses stack allocators to create stacks.
 */
template <typename Function, typename ExecutionContext,
          typename StackAllocator = boost::context::default_stack>
auto spawn(ExecutionContext& ctx, Function&& function,
           StackAllocator&& salloc = StackAllocator())
  -> typename std::enable_if<std::is_convertible<
       ExecutionContext&, detail::net::execution_context&>::value &&
       detail::is_stack_allocator<typename std::decay<StackAllocator>::type>::value>::type;

/*@}*/

} // namespace spawn

#include <spawn/impl/spawn.hpp>
