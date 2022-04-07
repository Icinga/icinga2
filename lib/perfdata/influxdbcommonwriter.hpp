/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#ifndef INFLUXDBCOMMONWRITER_H
#define INFLUXDBCOMMONWRITER_H

#include "perfdata/influxdbcommonwriter-ti.hpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/perfdatavalue.hpp"
#include "base/tcpsocket.hpp"
#include "base/timer.hpp"
#include "base/tlsstream.hpp"
#include "base/workqueue.hpp"
#include "remote/url.hpp"
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <atomic>
#include <fstream>

namespace icinga
{

/**
 * Common base class for InfluxDB v1/v2 writers.
 *
 * @ingroup perfdata
 */
class InfluxdbCommonWriter : public ObjectImpl<InfluxdbCommonWriter>
{
public:
	DECLARE_OBJECT(InfluxdbCommonWriter);

	template<class InfluxWriter>
	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void ValidateHostTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateServiceTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

	boost::beast::http::request<boost::beast::http::string_body> AssembleBaseRequest(String body);
	Url::Ptr AssembleBaseUrl();
	virtual boost::beast::http::request<boost::beast::http::string_body> AssembleRequest(String body) = 0;
	virtual Url::Ptr AssembleUrl() = 0;

private:
	boost::signals2::connection m_HandleCheckResults;
	Timer::Ptr m_FlushTimer;
	WorkQueue m_WorkQueue{10000000, 1};
	std::vector<String> m_DataBuffer;
	std::atomic_size_t m_DataBufferSize{0};

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void CheckResultHandlerWQ(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void SendMetric(const Checkable::Ptr& checkable, const Dictionary::Ptr& tmpl,
		const String& label, const Dictionary::Ptr& fields, double ts);
	void FlushTimeout();
	void FlushTimeoutWQ();
	void FlushWQ();

	static String EscapeKeyOrTagValue(const String& str);
	static String EscapeValue(const Value& value);

	OptionalTlsStream Connect();

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);
};

template<class InfluxWriter>
void InfluxdbCommonWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData nodes;
	auto typeName (InfluxWriter::TypeInstance->GetName().ToLower());

	for (const typename InfluxWriter::Ptr& influxwriter : ConfigType::GetObjectsByType<InfluxWriter>()) {
		size_t workQueueItems = influxwriter->m_WorkQueue.GetLength();
		double workQueueItemRate = influxwriter->m_WorkQueue.GetTaskCount(60) / 60.0;
		size_t dataBufferItems = influxwriter->m_DataBufferSize;

		nodes.emplace_back(influxwriter->GetName(), new Dictionary({
			{ "work_queue_items", workQueueItems },
			{ "work_queue_item_rate", workQueueItemRate },
			{ "data_buffer_items", dataBufferItems }
		}));

		perfdata->Add(new PerfdataValue(typeName + "_" + influxwriter->GetName() + "_work_queue_items", workQueueItems));
		perfdata->Add(new PerfdataValue(typeName + "_" + influxwriter->GetName() + "_work_queue_item_rate", workQueueItemRate));
		perfdata->Add(new PerfdataValue(typeName + "_" + influxwriter->GetName() + "_data_queue_items", dataBufferItems));
	}

	status->Set(typeName, new Dictionary(std::move(nodes)));
}

}

#endif /* INFLUXDBCOMMONWRITER_H */
