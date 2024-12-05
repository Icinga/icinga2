/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/configuration.hpp"
#include "base/debug.hpp"
#include "base/exception.hpp"
#include "base/io-engine.hpp"
#include "base/lazy-init.hpp"
#include "base/logger.hpp"
#include <exception>
#include <memory>
#include <thread>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/post.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/system/error_code.hpp>

using namespace icinga;

CpuBoundWork::CpuBoundWork(boost::asio::yield_context yc)
	: m_Done(false)
{
	auto& ioEngine (IoEngine::Get());

	for (;;) {
		auto availableSlots (ioEngine.m_CpuBoundSemaphore.fetch_sub(1));

		if (availableSlots < 1) {
			ioEngine.m_CpuBoundSemaphore.fetch_add(1);
			IoEngine::YieldCurrentCoroutine(yc);
			continue;
		}

		break;
	}
}

void CpuBoundWork::Done()
{
	if (!m_Done) {
		IoEngine::Get().m_CpuBoundSemaphore.fetch_add(1);

		m_Done = true;
	}
}

LazyInit<std::unique_ptr<IoEngine>> IoEngine::m_Instance ([]() { return std::unique_ptr<IoEngine>(new IoEngine()); });

IoEngine& IoEngine::Get()
{
	return *m_Instance.Get();
}

boost::asio::io_context& IoEngine::GetIoContext()
{
	return m_IoContext;
}

IoEngine::IoEngine() : m_IoContext(), m_KeepAlive(boost::asio::make_work_guard(m_IoContext)), m_Threads(decltype(m_Threads)::size_type(Configuration::Concurrency * 2u)), m_AlreadyExpiredTimer(m_IoContext)
{
	m_AlreadyExpiredTimer.expires_at(boost::posix_time::neg_infin);
	m_CpuBoundSemaphore.store(Configuration::Concurrency * 3u / 2u);

	for (auto& thread : m_Threads) {
		thread = std::thread(&IoEngine::RunEventLoop, this);
	}
}

IoEngine::~IoEngine()
{
	for (auto& thread : m_Threads) {
		boost::asio::post(m_IoContext, []() {
			throw TerminateIoThread();
		});
	}

	for (auto& thread : m_Threads) {
		thread.join();
	}
}

void IoEngine::RunEventLoop()
{
	for (;;) {
		try {
			m_IoContext.run();

			break;
		} catch (const TerminateIoThread&) {
			break;
		} catch (const std::exception& e) {
			Log(LogCritical, "IoEngine", "Exception during I/O operation!");
			Log(LogDebug, "IoEngine") << "Exception during I/O operation: " << DiagnosticInformation(e);
		}
	}
}

AsioEvent::AsioEvent(boost::asio::io_context& io, bool init)
	: m_Timer(io)
{
	m_Timer.expires_at(init ? boost::posix_time::neg_infin : boost::posix_time::pos_infin);
}

void AsioEvent::Set()
{
	m_Timer.expires_at(boost::posix_time::neg_infin);
}

void AsioEvent::Clear()
{
	m_Timer.expires_at(boost::posix_time::pos_infin);
}

void AsioEvent::Wait(boost::asio::yield_context yc)
{
	boost::system::error_code ec;
	m_Timer.async_wait(yc[ec]);
}

AsioConditionVariable::AsioConditionVariable(boost::asio::io_context& io)
	: m_Timer(io)
{
	m_Timer.expires_at(boost::posix_time::pos_infin);
}

namespace icinga
{

struct UnlockingYieldContext
{
	/*UnlockingYieldContext(boost::asio::yield_context yc, std::unique_lock<std::mutex>* lock)
		: YC(std::move(yc)), Lock(lock)
	{
	}*/

	boost::asio::yield_context YC;
	std::unique_lock<std::mutex>* Lock;
};

template<class H>
class UnlockingYcHandler
{
public:
	UnlockingYcHandler(UnlockingYieldContext uyc)
		: m_Handler(uyc.YC), m_Lock(uyc.Lock)
	{
	}

	template<class... Args>
	auto operator()(Args&&... args) -> decltype(m_Handler(std::forward<Args>(args)...))
	{
		m_Lock->unlock();
		return m_Handler(std::forward<Args>(args)...);
	}

private:
	H m_Handler;
	std::unique_lock<std::mutex>* m_Lock;
};

}

template<class Signature>
class boost::asio::async_result<UnlockingYieldContext, Signature>
{
public:
	using BaseType = async_result<yield_context, Signature>;
	using completion_handler_type = UnlockingYcHandler<typename BaseType::completion_handler_type>;
	using return_type = typename BaseType::return_type;

	template<class... Args>
	explicit async_result(Args&&... args) : m_Result(std::forward<Args>(args)...)
	{
	}

	return_type get()
	{
		return m_Result.get();
	}

private:
	BaseType m_Result;
};

void AsioConditionVariable::Wait(std::unique_lock<std::mutex>& lock, boost::asio::yield_context yc)
{
	VERIFY(lock);
	boost::system::error_code ec;
	m_Timer.async_wait(UnlockingYieldContext{yc[ec], &lock});
}

bool AsioConditionVariable::NotifyOne(std::mutex& mutex)
{
	boost::system::error_code ec;
	std::unique_lock lock (mutex);
	return m_Timer.cancel_one(ec);
}

size_t AsioConditionVariable::NotifyAll(std::mutex& mutex)
{
	boost::system::error_code ec;
	std::unique_lock lock (mutex);
	return m_Timer.cancel(ec);
}

void Timeout::Cancel()
{
	m_Cancelled.store(true);

	boost::system::error_code ec;
	m_Timer.cancel(ec);
}
