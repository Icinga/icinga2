/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef GRAPHITEWRITER_H
#define GRAPHITEWRITER_H

#include "perfdata/graphitewriter-ti.hpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/tcpsocket.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include <fstream>
#include <mutex>

namespace icinga
{

/**
 * An Icinga graphite writer.
 *
 * @ingroup perfdata
 */
class GraphiteWriter final : public ObjectImpl<GraphiteWriter>
{
public:
	DECLARE_OBJECT(GraphiteWriter);
	DECLARE_OBJECTNAME(GraphiteWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void ValidateHostNameTemplate(const Lazy<String>& lvalue, const ValidationUtils& utils) override;
	void ValidateServiceNameTemplate(const Lazy<String>& lvalue, const ValidationUtils& utils) override;

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

private:
	Shared<AsioTcpStream>::Ptr m_Stream;
	std::mutex m_StreamMutex;
	WorkQueue m_WorkQueue{10000000, 1};

	Timer::Ptr m_ReconnectTimer;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void CheckResultHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void SendMetric(const Checkable::Ptr& checkable, const String& prefix, const String& name, double value, double ts);
	void SendPerfdata(const Checkable::Ptr& checkable, const String& prefix, const CheckResult::Ptr& cr, double ts);
	static String EscapeMetric(const String& str);
	static String EscapeMetricLabel(const String& str);
	static Value EscapeMacroMetric(const Value& value);

	void ReconnectTimerHandler();

	void Disconnect();
	void DisconnectInternal();
	void Reconnect();
	void ReconnectInternal();

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);
};

}

#endif /* GRAPHITEWRITER_H */
