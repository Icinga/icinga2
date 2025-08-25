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

/**
 * Acquires a slot for CPU-bound work.
 *
 * If and as long as the lock-free TryAcquireSlot() doesn't succeed,
 * subscribes to the slow path by waiting on a condition variable.
 * It is woken up by Done() which is called by the destructor.
 *
 * @param yc Needed to asynchronously wait for the condition variable.
 * @param strand Where to post the wake-up of the condition variable.
 */
CpuBoundWork::CpuBoundWork(boost::asio::yield_context yc, boost::asio::io_context::strand& strand)
	: m_Done(false)
{
	VERIFY(strand.running_in_this_thread());

	auto& ie (IoEngine::Get());
	Shared<AsioConditionVariable>::Ptr cv;

	while (!TryAcquireSlot()) {
		if (!cv) {
			cv = Shared<AsioConditionVariable>::Make(ie.GetIoContext());
		}

		{
			std::unique_lock lock (ie.m_CpuBoundWaitingMutex);

			// The above lines may take a little bit, so let's optimistically re-check.
			// Also mitigate lost wake-ups by re-checking during the lock:
			//
			// During our lock, Done() can't retrieve the subscribers to wake up,
			// so any ongoing wake-up is either done at this point or has not started yet.
			// If such a wake-up is done, it's a lost wake-up to us unless we re-check here
			// whether the slot being freed (just before the wake-up) is still available.
			if (TryAcquireSlot()) {
				break;
			}

			// If the (hypothetical) slot mentioned above was taken by another coroutine,
			// there are no free slots again, just as if no wake-ups happened just now.
			ie.m_CpuBoundWaiting.emplace_back(strand, cv);
		}

		cv->Wait(yc);
	}
}

/**
 * Tries to acquire a slot for CPU-bound work.
 *
 * Specifically, decrements the number of free slots (semaphore) by one,
 * but only if it's currently greater than zero.
 * Not falling below zero requires an atomic#compare_exchange_weak() loop
 * instead of a simple atomic#fetch_sub() call, but it's also atomic.
 *
 * @return Whether a slot was acquired.
 */
bool CpuBoundWork::TryAcquireSlot()
{
	auto& ie (IoEngine::Get());
	auto freeSlots (ie.m_CpuBoundSemaphore.load());

	while (freeSlots > 0u) {
		// If ie.m_CpuBoundSemaphore was changed after the last load,
		// compare_exchange_weak() will load its latest value into freeSlots for us to retry until...
		if (ie.m_CpuBoundSemaphore.compare_exchange_weak(freeSlots, freeSlots - 1u)) {
			// ... either we successfully decrement ie.m_CpuBoundSemaphore by one, ...
			return true;
		}
	}

	// ... or it becomes zero due to another coroutine.
	return false;
}

/**
 * Releases the own slot acquired by the constructor (TryAcquireSlot()) if not already done.
 *
 * Precisely, increments the number of free slots (semaphore) by one.
 * Also wakes up all waiting constructors (slow path) if necessary.
 */
void CpuBoundWork::Done()
{
	if (!m_Done) {
		m_Done = true;

		auto& ie (IoEngine::Get());

		// The constructor takes the slow path only if the semaphore is full,
		// so we only have to wake up constructors if the semaphore was full.
		// This works because after fetch_add(), TryAcquireSlot() (fast path) will succeed.
		if (ie.m_CpuBoundSemaphore.fetch_add(1) == 0u) {
			// So now there are only slow path subscribers from just before the fetch_add() to be woken up.
			// Precisely, only subscribers from just before the fetch_add() which turned 0 to 1.

			decltype(ie.m_CpuBoundWaiting) subscribers;

			{
				// Locking after fetch_add() is safe because a delayed wake-up is fine.
				// Wake-up of constructors which subscribed after the fetch_add() is also not a problem.
				// In worst case, they will just re-subscribe to the slow path.
				// Lost wake-ups are mitigated by the constructor, see its implementation comments.
				std::unique_lock lock (ie.m_CpuBoundWaitingMutex);
				std::swap(subscribers, ie.m_CpuBoundWaiting);
			}

			// Again, a delayed wake-up is fine, hence unlocked.
			for (auto& [strand, cv] : subscribers) {
				boost::asio::post(strand, [cv = std::move(cv)] { cv->NotifyOne(); });
			}
		}
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
