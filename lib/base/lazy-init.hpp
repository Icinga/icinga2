/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef LAZY_INIT
#define LAZY_INIT

#include <atomic>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <functional>
#include <mutex>
#include <utility>

namespace icinga
{

/**
 * Lazy object initialization abstraction inspired from
 * <https://docs.microsoft.com/en-us/dotnet/api/system.lazy-1?view=netframework-4.7.2>.
 *
 * @ingroup base
 */
template<class T>
class LazyInit
{
public:
	inline
	LazyInit(std::function<T()> initializer = []() { return T(); }) : m_Initializer(std::move(initializer))
	{
		m_Underlying.store(nullptr, std::memory_order_release);
	}

	LazyInit(const LazyInit&) = delete;
	LazyInit(LazyInit&&) = delete;
	LazyInit& operator=(const LazyInit&) = delete;
	LazyInit& operator=(LazyInit&&) = delete;

	inline
	~LazyInit()
	{
		auto ptr (m_Underlying.load(std::memory_order_acquire));

		if (ptr != nullptr) {
			delete ptr;
		}
	}

	inline
	T& Get()
	{
		auto ptr (m_Underlying.load(std::memory_order_acquire));

		if (ptr == nullptr) {
			std::unique_lock<std::mutex> lock (m_Mutex);

			ptr = m_Underlying.load(std::memory_order_acquire);

			if (ptr == nullptr) {
				ptr = new T(m_Initializer());
				m_Underlying.store(ptr, std::memory_order_release);
			}
		}

		return *ptr;
	}

private:
	std::function<T()> m_Initializer;
	std::mutex m_Mutex;
	std::atomic<T*> m_Underlying;
};

/**
 * Seamless wrapper for boost::intrusive_ptr<T> which auto-creates the referenced object.
 *
 * @ingroup base
 */
template<class T>
class LazyPtr : public boost::intrusive_ptr<T>
{
public:
	using boost::intrusive_ptr<T>::intrusive_ptr;

	// Prevent *(*(decltype(*this)const*)this) and (*(decltype(*this)const*)this)->
	// to call parent methods which don't auto-create the referenced object
	T& operator*() const BOOST_SP_NOEXCEPT_WITH_ASSERT = delete;
	T* operator->() const BOOST_SP_NOEXCEPT_WITH_ASSERT = delete;

	inline T& operator*()
	{
		return *GetOrCreateObject();
	}

	inline T* operator->()
	{
		return GetOrCreateObject();
	}

private:
	auto GetOrCreateObject()
	{
		if (!this->get()) {
			this->reset(new T());
		}

		return this->get();
	}
};

}

#endif /* LAZY_INIT */
