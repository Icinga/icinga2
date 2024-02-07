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

AsioDualEvent::AsioDualEvent(boost::asio::io_context& io, bool init)
	: m_IsTrue(io, init), m_IsFalse(io, !init)
{
}

void AsioDualEvent::Set()
{
	m_IsTrue.Set();
	m_IsFalse.Clear();
}

void AsioDualEvent::Clear()
{
	m_IsTrue.Clear();
	m_IsFalse.Set();
}

void AsioDualEvent::WaitForSet(boost::asio::yield_context yc)
{
	m_IsTrue.Wait(std::move(yc));
}

void AsioDualEvent::WaitForClear(boost::asio::yield_context yc)
{
	m_IsFalse.Wait(std::move(yc));
}

/**
 * Cancels any pending timeout callback.
 *
 * Must be called in the strand in which the callback was scheduled!
 */
void Timeout::Cancel()
{
	m_Cancelled->store(true);

	boost::system::error_code ec;
	m_Timer.cancel(ec);
}
