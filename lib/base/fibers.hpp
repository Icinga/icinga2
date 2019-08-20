/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef FIBERS_H
#define FIBERS_H

#include "base/debug.hpp"
#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <cstdint>
#include <exception>
#include <memory>
#include <thread>
#include <utility>

/* Boost.Fiber scheduler with Boost.ASIO integration
 *
 * Usage:
 *
 * 1. Make sure to have done all fork(2)s.
 *    If a process fork(2)s child processes after performing any of the following steps,
 *    these children shall close all inherited file descriptors
 *    which may influence this component running in the parent process ASAP
 *    and these children shall not perform any of the following steps.
 * 2. Call StartEngine() once. (This initializes Io.)
 * 3. Spawn fibers as necessary. These fibers may run async I/O ops as in Example().
 */

namespace icinga
{

namespace Fibers
{

/**
 * For the black magic see below (boost::asio::*).
 */
class Yield
{
public:
	/**
	 * Cheats the compiler on `static_assert failed "ReadHandler type requirements not met"`.
	 */
	template<class... Args>
	inline void operator()(Args&&...)
	{
		VERIFY(!"icinga::Fibers::Yield#operator()<Unspecified...>() called");
		std::terminate();
	}
};

extern std::unique_ptr<boost::asio::io_context> Io;

void StartEngine(uint32_t threads = std::thread::hardware_concurrency());

} /* Fibers */

extern Fibers::Yield yf;

} /* icinga */

namespace boost
{
namespace asio
{

template<class NotImplemented>
struct async_completion<icinga::Fibers::Yield, NotImplemented>
{
};

template<class NotImplemented>
class async_result<icinga::Fibers::Yield, NotImplemented>
{
};

template<class Arg2>
struct async_completion<icinga::Fibers::Yield, void(system::error_code, Arg2)>
{
	class completion_handler_type
	{
	public:
		inline completion_handler_type(fibers::promise<Arg2>& promise)
			: m_Promise(&promise)
		{
		}

		void operator()(system::error_code ec, Arg2 res)
		{
			if (ec) {
				try {
					throw system::system_error(std::move(ec));
				} catch (const system::system_error&) {
					m_Promise->set_exception(std::current_exception());
					return;
				}
			}

			m_Promise->set_value(std::move(res));
		}

	private:
		fibers::promise<Arg2>* m_Promise;
	};

	inline async_completion(icinga::Fibers::Yield& y)
		: completion_handler(result.m_Promise), result(y)
	{
	}

	async_completion(const async_completion&) = delete;
	async_completion(async_completion&&) = delete;
	async_completion& operator=(const async_completion&) = delete;
	async_completion& operator=(async_completion&&) = delete;

	completion_handler_type completion_handler;
	async_result<icinga::Fibers::Yield, void(system::error_code, Arg2)> result;
};

template<class Arg2>
class async_result<icinga::Fibers::Yield, void(system::error_code, Arg2)>
{
	friend async_completion<icinga::Fibers::Yield, void(system::error_code, Arg2)>;

public:
	typedef icinga::Fibers::Yield completion_handler_type;
	typedef Arg2 return_type;

	inline async_result(completion_handler_type&)
	{
	}

	async_result(const async_result&) = delete;
	async_result(async_result&&) = delete;
	async_result& operator=(const async_result&) = delete;
	async_result& operator=(async_result&&) = delete;

	return_type get()
	{
		auto future (m_Promise.get_future());

		while (!future.valid()) {
			if (!icinga::Fibers::Io->poll_one()) {
				this_fiber::yield();
			}
		}

		return future.get();
	}

private:
	fibers::promise<return_type> m_Promise;
};

} /* asio */
} /* boost */

#ifndef _WIN32
namespace icinga
{
namespace Fibers
{

/**
 * Illustrates the usage of yf inside fibers.
 */
static inline
void Example()
{
	namespace asio = boost::asio;
	namespace fibers = boost::fibers;
	namespace sys = boost::system;
	namespace this_fiber = boost::this_fiber;

	assert(this_fiber::get_id() != fibers::fiber::id());

	asio::posix::stream_descriptor stdin (*Io, STDIN_FILENO);
	char buf[42] = {};

	try {
		std::size_t amount = stdin.async_read_some(asio::mutable_buffer(buf, sizeof(buf) / sizeof(buf[0])), yf);
	} catch (const sys::system_error&) {
	}

	stdin.release();
}

} /* Fibers */
} /* icinga */
#endif /* _WIN32 */

#endif /* FIBERS_H */
