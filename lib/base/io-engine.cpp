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

CpuBoundWork::CpuBoundWork(boost::asio::yield_context yc, boost::asio::io_context::strand& strand)
	: m_Done(false)
{
	auto& ioEngine (IoEngine::Get());
	auto& sem (ioEngine.m_CpuBoundSemaphore);
	std::unique_lock<std::mutex> lock (sem.Mutex);

	if (sem.FreeSlots) {
		--sem.FreeSlots;
		return;
	}

	auto cv (Shared<AsioConditionVariable>::Make(ioEngine.GetIoContext()));
	bool gotSlot = false;
	auto pos (sem.Waiting.insert(sem.Waiting.end(), IoEngine::CpuBoundQueueItem{&strand, cv, &gotSlot}));

	lock.unlock();

	try {
		cv->Wait(yc);
	} catch (...) {
		std::unique_lock<std::mutex> lock (sem.Mutex);

		if (gotSlot) {
			lock.unlock();
			Done();
		} else {
			sem.Waiting.erase(pos);
		}

		throw;
	}
}

void CpuBoundWork::Done()
{
	if (!m_Done) {
		auto& sem (IoEngine::Get().m_CpuBoundSemaphore);
		std::unique_lock<std::mutex> lock (sem.Mutex);

		if (sem.Waiting.empty()) {
			++sem.FreeSlots;
		} else {
			auto next (sem.Waiting.front());

			*next.GotSlot = true;
			sem.Waiting.pop_front();
			boost::asio::post(*next.Strand, [cv = std::move(next.CV)]() { cv->Set(); });
		}

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

	{
		std::unique_lock<std::mutex> lock (m_CpuBoundSemaphore.Mutex);
		m_CpuBoundSemaphore.FreeSlots = Configuration::Concurrency * 3u / 2u;
	}

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
