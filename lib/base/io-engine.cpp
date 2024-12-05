/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/configuration.hpp"
#include "base/exception.hpp"
#include "base/io-engine.hpp"
#include "base/lazy-init.hpp"
#include "base/logger.hpp"
#include "base/shared.hpp"
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
	VERIFY(strand.running_in_this_thread());

	auto& ie (IoEngine::Get());
	Shared<AsioConditionVariable>::Ptr cv;

	for (;;) {
		for (auto freeSlots (ie.m_CpuBoundSemaphore.load()); freeSlots > 0;) {
			if (ie.m_CpuBoundSemaphore.compare_exchange_weak(freeSlots, freeSlots - 1)) {
				return;
			}
		}

		if (!cv) {
			cv = Shared<AsioConditionVariable>::Make(ie.GetIoContext());
			continue;
		}

		boost::asio::post(ie.m_CpuBoundStrand, [&ie, strand, cv] {
			auto f = [strand, cv](boost::system::error_code) {
				boost::asio::post(strand, [cv] { cv->NotifyOne(); });
			};

			if (ie.m_CpuBoundSemaphore.load() > 0) {
				f(boost::system::error_code());
			} else {
				ie.m_CpuBoundCv.async_wait(boost::asio::bind_executor(ie.m_CpuBoundStrand, std::move(f)));
			}
		});

		cv->Wait(yc);
	}
}

void CpuBoundWork::Done()
{
	if (!m_Done) {
		m_Done = true;

		auto& ie (IoEngine::Get());
		ie.m_CpuBoundSemaphore.fetch_add(1);

		boost::asio::post(ie.m_CpuBoundStrand, [&ie] {
			boost::system::error_code ec;
			ie.m_CpuBoundCv.cancel(ec);
		});
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

IoEngine::IoEngine()
	: m_KeepAlive(boost::asio::make_work_guard(m_IoContext)), m_Threads(decltype(m_Threads)::size_type(Configuration::Concurrency * 2u)),
	  m_AlreadyExpiredTimer(m_IoContext), m_CpuBoundCv(m_IoContext), m_CpuBoundStrand(m_IoContext)
{
	m_AlreadyExpiredTimer.expires_at(boost::posix_time::neg_infin);
	m_CpuBoundSemaphore.store(Configuration::Concurrency * 3u / 2u);
	m_CpuBoundCv.expires_at(boost::posix_time::pos_infin);

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

void AsioConditionVariable::Wait(boost::asio::yield_context yc)
{
	boost::system::error_code ec;
	m_Timer.async_wait(yc[ec]);
}

bool AsioConditionVariable::NotifyOne()
{
	boost::system::error_code ec;
	return m_Timer.cancel_one(ec);
}

size_t AsioConditionVariable::NotifyAll()
{
	boost::system::error_code ec;
	return m_Timer.cancel(ec);
}

void Timeout::Cancel()
{
	m_Cancelled.store(true);

	boost::system::error_code ec;
	m_Timer.cancel(ec);
}
