/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#ifndef BULKER_H
#define BULKER_H

#include <boost/config.hpp>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

namespace icinga
{

/**
 * A queue which outputs the input as bulks of a defined size
 * or after a defined time, whichever is reached first
 *
 * @ingroup base
 */
template<class T>
class Bulker
{
private:
	typedef std::chrono::steady_clock Clock;

public:
	typedef std::vector<T> Container;
	typedef typename Container::size_type SizeType;
	typedef typename Clock::duration Duration;

	Bulker(SizeType bulkSize, Duration threshold)
		: m_BulkSize(bulkSize), m_Threshold(threshold), m_NextConsumption(NullTimePoint()) { }

	void ProduceOne(T needle);
	Container ConsumeMany();
	SizeType Size();

	inline SizeType GetBulkSize() const noexcept
	{
		return m_BulkSize;
	}

private:
	typedef std::chrono::time_point<Clock> TimePoint;

	static inline
	TimePoint NullTimePoint()
	{
		return TimePoint::min();
	}

	inline void UpdateNextConsumption()
	{
		m_NextConsumption = Clock::now() + m_Threshold;
	}

	const SizeType m_BulkSize;
	const Duration m_Threshold;

	std::mutex m_Mutex;
	std::condition_variable m_CV;
	std::queue<Container> m_Bulks;
	TimePoint m_NextConsumption;
};

template<class T>
void Bulker<T>::ProduceOne(T needle)
{
	std::unique_lock<std::mutex> lock (m_Mutex);

	if (m_Bulks.empty() || m_Bulks.back().size() == m_BulkSize) {
		m_Bulks.emplace();
	}

	m_Bulks.back().emplace_back(std::move(needle));

	if (m_Bulks.size() == 1u && m_Bulks.back().size() == m_BulkSize) {
		m_CV.notify_one();
	}
}

template<class T>
typename Bulker<T>::Container Bulker<T>::ConsumeMany()
{
	std::unique_lock<std::mutex> lock (m_Mutex);

	if (BOOST_UNLIKELY(m_NextConsumption == NullTimePoint())) {
		UpdateNextConsumption();
	}

	auto deadline (m_NextConsumption);

	m_CV.wait_until(lock, deadline, [this]() { return !m_Bulks.empty() && m_Bulks.front().size() == m_BulkSize; });
	UpdateNextConsumption();

	if (m_Bulks.empty()) {
		return Container();
	}

	auto haystack (std::move(m_Bulks.front()));

	m_Bulks.pop();
	return haystack;
}

template<class T>
typename Bulker<T>::SizeType Bulker<T>::Size()
{
	std::unique_lock<std::mutex> lock (m_Mutex);

	return m_Bulks.empty() ? 0 : (m_Bulks.size() - 1u) * m_BulkSize + m_Bulks.back().size();
}

}

#endif /* BULKER_H */
