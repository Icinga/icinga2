//
// impl/spawn.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2017 Oliver Kowalke (oliver dot kowalke at gmail dot com)
// Copyright (c) 2019 Casey Bodley (cbodley at redhat dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <atomic>
#include <memory>

#include <boost/system/system_error.hpp>
#include <boost/context/continuation.hpp>

#include <spawn/detail/net.hpp>
#include <spawn/detail/is_stack_allocator.hpp>

namespace spawn::detail {

  class continuation_context
  {
  public:
    boost::context::continuation context_;

    continuation_context()
      : context_()
    {
    }

    void resume()
    {
      context_ = context_.resume();
    }
  };

  template <typename Handler, typename T>
  class coro_handler
  {
  public:
    coro_handler(basic_yield_context<Handler> ctx)
      : callee_(ctx.callee_.lock()),
        caller_(ctx.caller_),
        handler_(ctx.handler_),
        ready_(0),
        ec_(ctx.ec_),
        value_(0)
    {
    }

    void operator()(T value)
    {
      *ec_ = boost::system::error_code();
      *value_ = std::move(value);
      if (--*ready_ == 0)
        callee_->resume();
    }

    void operator()(boost::system::error_code ec, T value)
    {
      *ec_ = ec;
      *value_ = std::move(value);
      if (--*ready_ == 0)
        callee_->resume();
    }

  //private:
    std::shared_ptr<continuation_context> callee_;
    continuation_context& caller_;
    Handler handler_;
    std::atomic<long>* ready_;
    boost::system::error_code* ec_;
    T* value_;
  };

  template <typename Handler>
  class coro_handler<Handler, void>
  {
  public:
    coro_handler(basic_yield_context<Handler> ctx)
      : callee_(ctx.callee_.lock()),
        caller_(ctx.caller_),
        handler_(ctx.handler_),
        ready_(0),
        ec_(ctx.ec_)
    {
    }

    void operator()()
    {
      *ec_ = boost::system::error_code();
      if (--*ready_ == 0)
        callee_->resume();
    }

    void operator()(boost::system::error_code ec)
    {
      *ec_ = ec;
      if (--*ready_ == 0)
        callee_->resume();
    }

  //private:
    std::shared_ptr<continuation_context> callee_;
    continuation_context& caller_;
    Handler handler_;
    std::atomic<long>* ready_;
    boost::system::error_code* ec_;
  };

  template <typename Handler, typename T>
  class coro_async_result
  {
  public:
    using completion_handler_type = coro_handler<Handler, T>;
    using return_type = T;

    explicit coro_async_result(completion_handler_type& h)
      : handler_(h),
        caller_(h.caller_),
        ready_(2)
    {
      h.ready_ = &ready_;
      out_ec_ = h.ec_;
      if (!out_ec_) h.ec_ = &ec_;
      h.value_ = &value_;
    }

    return_type get()
    {
      // Must not hold shared_ptr while suspended.
      handler_.callee_.reset();

      if (--ready_ != 0)
        caller_.resume(); // suspend caller
      if (!out_ec_ && ec_) throw boost::system::system_error(ec_);
      return std::move(value_);
    }

  private:
    completion_handler_type& handler_;
    continuation_context& caller_;
    std::atomic<long> ready_;
    boost::system::error_code* out_ec_;
    boost::system::error_code ec_;
    return_type value_;
  };

  template <typename Handler>
  class coro_async_result<Handler, void>
  {
  public:
    using completion_handler_type = coro_handler<Handler, void>;
    using return_type = void;

    explicit coro_async_result(completion_handler_type& h)
      : handler_(h),
        caller_(h.caller_),
        ready_(2)
    {
      h.ready_ = &ready_;
      out_ec_ = h.ec_;
      if (!out_ec_) h.ec_ = &ec_;
    }

    void get()
    {
      // Must not hold shared_ptr while suspended.
      handler_.callee_.reset();

      if (--ready_ != 0)
        caller_.resume(); // suspend caller
      if (!out_ec_ && ec_) throw boost::system::system_error(ec_);
    }

  private:
    completion_handler_type& handler_;
    continuation_context& caller_;
    std::atomic<long> ready_;
    boost::system::error_code* out_ec_;
    boost::system::error_code ec_;
  };

} // namespace spawn::detail

#if !defined(GENERATING_DOCUMENTATION)
namespace SPAWN_NET_NAMESPACE {

template <typename Handler, typename ReturnType>
class async_result<spawn::basic_yield_context<Handler>, ReturnType()>
  : public spawn::detail::coro_async_result<Handler, void>
{
public:
  explicit async_result(
    typename spawn::detail::coro_async_result<Handler,
      void>::completion_handler_type& h)
    : spawn::detail::coro_async_result<Handler, void>(h)
  {
  }
};

template <typename Handler, typename ReturnType, typename Arg1>
class async_result<spawn::basic_yield_context<Handler>, ReturnType(Arg1)>
  : public spawn::detail::coro_async_result<Handler, typename std::decay<Arg1>::type>
{
public:
  explicit async_result(
    typename spawn::detail::coro_async_result<Handler,
      typename std::decay<Arg1>::type>::completion_handler_type& h)
    : spawn::detail::coro_async_result<Handler, typename std::decay<Arg1>::type>(h)
  {
  }
};

template <typename Handler, typename ReturnType>
class async_result<spawn::basic_yield_context<Handler>,
    ReturnType(boost::system::error_code)>
  : public spawn::detail::coro_async_result<Handler, void>
{
public:
  explicit async_result(
    typename spawn::detail::coro_async_result<Handler,
      void>::completion_handler_type& h)
    : spawn::detail::coro_async_result<Handler, void>(h)
  {
  }
};

template <typename Handler, typename ReturnType, typename Arg2>
class async_result<spawn::basic_yield_context<Handler>,
    ReturnType(boost::system::error_code, Arg2)>
  : public spawn::detail::coro_async_result<Handler, typename std::decay<Arg2>::type>
{
public:
  explicit async_result(
    typename spawn::detail::coro_async_result<Handler,
      typename std::decay<Arg2>::type>::completion_handler_type& h)
    : spawn::detail::coro_async_result<Handler, typename std::decay<Arg2>::type>(h)
  {
  }
};

template <typename Handler, typename T, typename Allocator>
struct associated_allocator<spawn::detail::coro_handler<Handler, T>, Allocator>
{
  using type = associated_allocator_t<Handler, Allocator>;

  static type get(const spawn::detail::coro_handler<Handler, T>& h,
      const Allocator& a = Allocator()) noexcept
  {
    return associated_allocator<Handler, Allocator>::get(h.handler_, a);
  }
};

template <typename Handler, typename T, typename Executor>
struct associated_executor<spawn::detail::coro_handler<Handler, T>, Executor>
{
  using type = associated_executor_t<Handler, Executor>;

  static type get(const spawn::detail::coro_handler<Handler, T>& h,
      const Executor& ex = Executor()) noexcept
  {
    return associated_executor<Handler, Executor>::get(h.handler_, ex);
  }
};

} // namespace SPAWN_NET_NAMESPACE

namespace spawn {
namespace detail {

  template <typename Handler, typename Function, typename StackAllocator>
  struct spawn_data
  {
    template <typename Hand, typename Func, typename Stack>
    spawn_data(Hand&& handler, bool call_handler, Func&& function, Stack&& salloc)
      : handler_(std::forward<Hand>(handler)),
        call_handler_(call_handler),
        function_(std::forward<Func>(function)),
        salloc_(std::forward<Stack>(salloc))
    {
    }
    spawn_data(const spawn_data&) = delete;
    spawn_data& operator=(const spawn_data&) = delete;

    Handler handler_;
    bool call_handler_;
    Function function_;
    StackAllocator salloc_;
    continuation_context caller_;
  };

  template <typename Handler, typename Function, typename StackAllocator>
  struct spawn_helper
  {
    void operator()()
    {
      callee_.reset(new continuation_context());
      callee_->context_ = boost::context::callcc(
          std::allocator_arg, std::move(data_->salloc_),
          [this] (boost::context::continuation&& c)
          {
            std::shared_ptr<spawn_data<Handler, Function, StackAllocator> > data = data_;
            data->caller_.context_ = std::move(c);
            const basic_yield_context<Handler> yh(callee_, data->caller_, data->handler_);
            (data->function_)(yh);
            if (data->call_handler_)
            {
              (data->handler_)();
            }
            boost::context::continuation caller = std::move(data->caller_.context_);
            data.reset();
            return caller;
          });
    }

    std::shared_ptr<continuation_context> callee_;
    std::shared_ptr<spawn_data<Handler, Function, StackAllocator> > data_;
  };

  inline void default_spawn_handler() {}

} // namespace detail

template <typename Function, typename StackAllocator>
auto spawn(Function&& function, StackAllocator&& salloc)
  -> typename std::enable_if<detail::is_stack_allocator<
       typename std::decay<StackAllocator>::type>::value>::type
{
  auto ex = detail::net::get_associated_executor(function);

  spawn(ex, std::forward<Function>(function), salloc);
}

template <typename Handler, typename Function, typename StackAllocator>
auto spawn(Handler&& handler, Function&& function, StackAllocator&& salloc)
  -> typename std::enable_if<!detail::net::is_executor<typename std::decay<Handler>::type>::value &&
       !std::is_convertible<Handler&, detail::net::execution_context&>::value &&
       !detail::is_stack_allocator<typename std::decay<Function>::type>::value &&
       detail::is_stack_allocator<typename std::decay<StackAllocator>::type>::value>::type
{
  using handler_type = typename std::decay<Handler>::type;
  using function_type = typename std::decay<Function>::type;

  auto ex = detail::net::get_associated_executor(handler);
  auto a = detail::net::get_associated_allocator(handler);

  detail::spawn_helper<handler_type, function_type, StackAllocator> helper;
  helper.data_ = std::make_shared<
      detail::spawn_data<handler_type, function_type, StackAllocator> >(
        std::forward<Handler>(handler), true,
        std::forward<Function>(function),
        std::forward<StackAllocator>(salloc));

  ex.dispatch(helper, a);
}

template <typename Handler, typename Function, typename StackAllocator>
auto spawn(basic_yield_context<Handler> ctx, Function&& function,
           StackAllocator&& salloc)
  -> typename std::enable_if<detail::is_stack_allocator<
       typename std::decay<StackAllocator>::type>::value>::type
{
  using function_type = typename std::decay<Function>::type;

  Handler handler(ctx.handler_); // Explicit copy that might be moved from.

  auto ex = detail::net::get_associated_executor(handler);
  auto a = detail::net::get_associated_allocator(handler);

  detail::spawn_helper<Handler, function_type, StackAllocator> helper;
  helper.data_ = std::make_shared<
      detail::spawn_data<Handler, function_type, StackAllocator> >(
        std::forward<Handler>(handler), false,
        std::forward<Function>(function),
        std::forward<StackAllocator>(salloc));

  ex.dispatch(helper, a);
}

template <typename Function, typename Executor, typename StackAllocator>
auto spawn(const Executor& ex, Function&& function, StackAllocator&& salloc)
  -> typename std::enable_if<detail::net::is_executor<Executor>::value &&
       detail::is_stack_allocator<typename std::decay<StackAllocator>::type>::value>::type
{
  spawn(detail::net::strand<Executor>(ex),
      std::forward<Function>(function),
      std::forward<StackAllocator>(salloc));
}

template <typename Function, typename Executor, typename StackAllocator>
auto spawn(const detail::net::strand<Executor>& ex,
           Function&& function, StackAllocator&& salloc)
  -> typename std::enable_if<detail::is_stack_allocator<
       typename std::decay<StackAllocator>::type>::value>::type
{
  spawn(bind_executor(ex, &detail::default_spawn_handler),
      std::forward<Function>(function),
      std::forward<StackAllocator>(salloc));
}

template <typename Function, typename ExecutionContext, typename StackAllocator>
auto spawn(ExecutionContext& ctx, Function&& function, StackAllocator&& salloc)
  -> typename std::enable_if<std::is_convertible<
       ExecutionContext&, detail::net::execution_context&>::value &&
       detail::is_stack_allocator<typename std::decay<StackAllocator>::type>::value>::type
{
  spawn(ctx.get_executor(),
      std::forward<Function>(function),
      std::forward<StackAllocator>(salloc));
}

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace spawn
