/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/application.hpp"
#include "base/shared-object.hpp"
#include "base/threadpool.hpp"
#include <boost/asio/dispatch.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/spawn.hpp>
#include <future>

namespace boost::asio::detail {

struct fixed_throw_tag
{};

/**
 * Fixes the issue where operations crash the program that can throw exceptions but don't.
 *
 * The issues is that in the orginal version of this specialization, the exception_ptr is
 * never checked against nullptr, but only the pointer to the exception_ptr, which is likely
 * a mistake.
 */
template<typename Executor, typename R, typename T>
class spawn_handler<Executor, R(std::exception_ptr, T, fixed_throw_tag)> : public spawn_handler_base<Executor>
{
public:
	using return_type = T;

	struct result_type
	{
		std::exception_ptr ex_;
		return_type* value_;
	};

	spawn_handler(const basic_yield_context<Executor>& yield, result_type& result)
		: spawn_handler_base<Executor>(yield), result_(result)
	{
	}

	void operator()(std::exception_ptr ex, T value)
	{
		result_.ex_ = ex;
		result_.value_ = &value;
		this->resume();
	}

	static return_type on_resume(result_type& result)
	{
		if (result.ex_) {
			rethrow_exception(result.ex_);
		}
		return BOOST_ASIO_MOVE_CAST(return_type)(*result.value_);
	}

private:
	result_type& result_;
};

} // namespace boost::asio::detail

namespace icinga {

template<typename>
class AsioPromise;

/**
 * Implements a generic, asynchronously awaitable future.
 *
 * This allows to queue an CPU-intensive action on another thread without blocking any
 * IO-threads and pass back the result via the @c AsioPromise.
 *
 * Similar to @c std::future, this is single-use only. Once a value has been set by the
 * @c AsioPromise, the job is done.
 */
template<typename ValueType>
class AsioFuture : public SharedObject
{
	template<typename>
	friend class AsioPromise;

public:
	DECLARE_PTR_TYPEDEFS(AsioFuture);

	/**
	 * Returns the value held in the future, or waits for the promise to complete.
	 *
	 * If an exception has been stored in the future via AsioPromise::SetException(), it will be
	 * thrown by this function. Simply passing `yc[ec]` as a token will not change this, even if
	 * the exception that would be thrown is a @c boost::asio::system::system_error.
	 */
	template<typename CompletionToken>
	auto Get(CompletionToken&& token)
	{
		using Signature = void(std::exception_ptr, ValueType, boost::asio::detail::fixed_throw_tag);

		return boost::asio::async_initiate<CompletionToken, Signature>(
			[this](auto&& handler) { InitOperation(std::forward<decltype(handler)>(handler)); },
			std::forward<CompletionToken>(token)
		);
	}

	// TODO: Add WaitFor and WaitUntil

private:
	template<typename Handler>
	void CallHandler(Handler&& handler)
	{
		if (std::holds_alternative<ValueType>(m_Value)) {
			std::forward<Handler>(handler)(nullptr, std::get<ValueType>(m_Value));
		} else {
			std::forward<Handler>(handler)(std::get<std::exception_ptr>(m_Value), {});
		}
	}

	template<typename Handler>
	void InitOperation(Handler&& handler)
	{
		auto handlerPtr = std::make_shared<std::decay_t<decltype(handler)>>(std::forward<decltype(handler)>(handler));

		auto handlerWrapper = [handler = handlerPtr, future = AsioFuture::Ptr{this}]() {
			if (std::holds_alternative<ValueType>(future->m_Value)) {
				(*handler)({}, std::get<ValueType>(future->m_Value));
			} else {
				(*handler)(std::get<std::exception_ptr>(future->m_Value), {});
			}
		};

		std::unique_lock lock(m_Mutex);

		if (!std::holds_alternative<std::monostate>(m_Value)) {
			boost::asio::post(boost::asio::get_associated_executor(handler), handlerWrapper);
			return;
		}

		auto work = boost::asio::make_work_guard(handler);
		m_Callback = [handler = std::move(handlerWrapper), work = std::move(work)]() mutable {
			boost::asio::dispatch(work.get_executor(), handler);
			work.reset();
		};
	}

	std::mutex m_Mutex;
	std::variant<std::monostate, std::exception_ptr, ValueType> m_Value;
	std::function<void()> m_Callback;
};

/**
 * A promise type that can be passed to any other thread or coroutine.
 */
template<typename ValueType>
class AsioPromise
{
public:
	AsioPromise() : m_Future(new AsioFuture<ValueType>) {}

	template<typename ForwardingType>
	void SetValue(ForwardingType&& value) const
	{
		std::unique_lock lock{m_Future->m_Mutex};

		if (!std::holds_alternative<std::monostate>(m_Future->m_Value)) {
			BOOST_THROW_EXCEPTION(std::future_error{std::future_errc::promise_already_satisfied});
		}

		m_Future->m_Value = std::forward<ForwardingType>(value);
		if (m_Future->m_Callback) {
			m_Future->m_Callback();
		}
	}

	template<typename ExceptionType>
	void SetException(ExceptionType&& ex) const
	{
		std::unique_lock lock{m_Future->m_Mutex};

		if (!std::holds_alternative<std::monostate>(m_Future->m_Value)) {
			BOOST_THROW_EXCEPTION(std::future_error{std::future_errc::promise_already_satisfied});
		}

		m_Future->m_Value = std::make_exception_ptr(std::forward<ExceptionType>(ex));
		if (m_Future->m_Callback) {
			m_Future->m_Callback();
		}
	}

	auto GetFuture() const { return m_Future; }

private:
	typename AsioFuture<ValueType>::Ptr m_Future;
};

template<typename Callback>
auto QueueAsioFutureCallback(Callback&& cb)
{
	AsioPromise<decltype(cb())> promise;
	auto future = promise.GetFuture();
	Application::GetTP().Post(
		[cb = std::forward<Callback>(cb), promise = std::move(promise)]() {
			try {
				promise.SetValue(cb());
			} catch (const std::exception&) {
				promise.SetException(std::current_exception());
			}
		},
		{}
	);
	return future;
};

} // namespace icinga
