/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/configuration.hpp"
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

CpuBoundWork::~CpuBoundWork()
{
	if (!m_Done) {
		IoEngine::Get().m_CpuBoundSemaphore.fetch_add(1);
	}
}

void CpuBoundWork::Done()
{
	if (!m_Done) {
		IoEngine::Get().m_CpuBoundSemaphore.fetch_add(1);

		m_Done = true;
	}
}

IoBoundWorkSlot::IoBoundWorkSlot(boost::asio::yield_context yc)
	: yc(yc)
{
	IoEngine::Get().m_CpuBoundSemaphore.fetch_add(1);
}

IoBoundWorkSlot::~IoBoundWorkSlot()
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

AsioConditionVariable::AsioConditionVariable(boost::asio::io_context& io, bool init)
	: m_Timer(io)
{
	m_Timer.expires_at(init ? boost::posix_time::neg_infin : boost::posix_time::pos_infin);
}

void AsioConditionVariable::Set()
{
	m_Timer.expires_at(boost::posix_time::neg_infin);
}

void AsioConditionVariable::Clear()
{
	m_Timer.expires_at(boost::posix_time::pos_infin);
}

void AsioConditionVariable::Wait(boost::asio::yield_context yc)
{
	boost::system::error_code ec;
	m_Timer.async_wait(yc[ec]);
}

void Timeout::Cancel()
{
	m_Cancelled.store(true);

	boost::system::error_code ec;
	m_Timer.cancel(ec);
}

/**
 * Cancels the timeout and waits for the own coroutine to finish.
 *
 * @param yc Yield Context for ASIO
 */
void Timeout::Cancel(boost::asio::yield_context yc)
{
	Cancel();
	m_Done.Wait(yc);
}
