/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef INFLUXDBWRITER_H
#define INFLUXDBWRITER_H

#include "perfdata/influxdbwriter-ti.hpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/tcpsocket.hpp"
#include "base/timer.hpp"
#include "base/tlsstream.hpp"
#include "base/workqueue.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga InfluxDB writer.
 *
 * @ingroup perfdata
 */
class InfluxdbWriter final : public ObjectImpl<InfluxdbWriter>
{
public:
	DECLARE_OBJECT(InfluxdbWriter);
	DECLARE_OBJECTNAME(InfluxdbWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void ValidateHostTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateServiceTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;

	int GetQueryCount(RingBuffer::SizeType span);
	int GetPendingQueries();

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

private:
	WorkQueue m_WorkQueue{10000000, 1};
	Timer::Ptr m_FlushTimer;
	std::vector<String> m_DataBuffer;

	RingBuffer m_QueryStats{15 * 60};

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void CheckResultHandlerWQ(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void SendMetric(const Checkable::Ptr& checkable, const Dictionary::Ptr& tmpl,
		const String& label, const Dictionary::Ptr& fields, double ts);
	void FlushTimeout();
	void FlushTimeoutWQ();
	void Flush();

	static String EscapeKeyOrTagValue(const String& str);
	static String EscapeValue(const Value& value);

	OptionalTlsStream Connect();

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);
};

}

#endif /* INFLUXDBWRITER_H */
