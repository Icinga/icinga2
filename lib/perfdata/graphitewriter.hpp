// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GRAPHITEWRITER_H
#define GRAPHITEWRITER_H

#include "perfdata/graphitewriter-ti.hpp"
#include "icinga/checkable.hpp"
#include "base/configobject.hpp"
#include "base/workqueue.hpp"
#include "perfdata/perfdatawriterconnection.hpp"

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
	PerfdataWriterConnection::Ptr m_Connection;
	WorkQueue m_WorkQueue{10000000, 1};

	boost::signals2::connection m_HandleCheckResults;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void SendMetric(const Checkable::Ptr& checkable, const String& prefix, const String& name, double value, double ts);
	void SendPerfdata(const Checkable::Ptr& checkable, const String& prefix, const CheckResult::Ptr& cr);
	static String EscapeMetric(const String& str);
	static String EscapeMetricLabel(const String& str);
	static Value EscapeMacroMetric(const Value& value);

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);
};

}

#endif /* GRAPHITEWRITER_H */
