// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "perfdata/otlpmetricswriter-ti.hpp"
#include "base/workqueue.hpp"
#include "icinga/checkable.hpp"
#include "otel/otel.hpp"

namespace icinga
{

class OTLPMetricsWriter final : public ObjectImpl<OTLPMetricsWriter>
{
public:
	DECLARE_OBJECT(OTLPMetricsWriter);
	DECLARE_OBJECTNAME(OTLPMetricsWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void ValidatePort(const Lazy<int>& lvalue, const ValidationUtils& utils) override;
	void ValidateFlushInterval(const Lazy<int>& lvalue, const ValidationUtils& utils) override;
	void ValidateFlushThreshold(const Lazy<int64_t>& lvalue, const ValidationUtils& utils) override;

protected:
	void OnAllConfigLoaded() override;
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

private:
	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void Flush(bool fromTimer = false);
	void AddBytesAndFlushIfNeeded(std::size_t newBytes = 0);

	template<typename T>
	[[nodiscard]] std::size_t Record(
		const Checkable::Ptr& checkable,
		std::string_view metric,
		T value,
		double startTime,
		double endTime,
		OTel::AttrsMap& attrs
	);

	std::atomic_uint64_t m_RecordedBytes{0}; // Total bytes recorded in the current OTel message.
	std::atomic_uint64_t m_DataPointsCount{0}; // Total data points recorded in the current OTel message.

	struct CheckableMetrics
	{
		/**
		 * The ResourceMetrics Protobuf object for this checkable.
		 *
		 * This object holds a pre-populated OTel Resource with all the necessary attributes for a given
		 * checkable till we flush the metrics to the OTel collector. On flush, we transform all the recorded
		 * metrics from @c Metrics into this Protobuf object and transfer ownership of it to the export request.
		 */
		std::unique_ptr<opentelemetry::proto::metrics::v1::ResourceMetrics> ResourceMetrics;
		String ServiceInstanceId; // Cached service.instance.id attribute value.
	};
	std::map<Checkable::Ptr, CheckableMetrics> m_Metrics;

	WorkQueue m_WorkQueue{10'000'000, 1};
	boost::signals2::connection m_CheckResultsSlot, m_ActiveChangedSlot;
	OTel::Ptr m_Exporter;
	Timer::Ptr m_FlushTimer;
	std::atomic_bool m_TimerFlushInProgress{false}; // Whether a timer-initiated flush is in progress.
};

} // namespace icinga
