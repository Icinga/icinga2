/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ELASTICSEARCHDATASTREAMWRITER_H
#define ELASTICSEARCHDATASTREAMWRITER_H

#include "perfdata/elasticsearchdatastreamwriter-ti.hpp"
#include "icinga/checkable.hpp"
#include "icinga/checkresult.hpp"
#include "base/configobject.hpp"
#include "base/workqueue.hpp"
#include "base/timer.hpp"
#include "base/tlsstream.hpp"

namespace icinga
{

class EcsDocument final : public ObjectImpl<EcsDocument> {
	public:
		DECLARE_PTR_TYPEDEFS(EcsDocument);

		EcsDocument() = default;
		EcsDocument(String index, Dictionary::Ptr document);
};

class ElasticsearchDatastreamWriter final : public ObjectImpl<ElasticsearchDatastreamWriter>
{
public:
	DECLARE_OBJECT(ElasticsearchDatastreamWriter);
	DECLARE_OBJECTNAME(ElasticsearchDatastreamWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static String FormatTimestamp(double ts);
	static String FormatIcingaVersion(unsigned long version);

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

private:
	String m_EventPrefix;
	WorkQueue m_WorkQueue{10000000, 1};
	boost::signals2::connection m_HandleCheckResults;
	Timer::Ptr m_FlushTimer;
	std::vector<EcsDocument::Ptr> m_DataBuffer;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);

	Dictionary::Ptr ExtractPerfData(const Checkable::Ptr checkable, const Array::Ptr& perfdata);

	OptionalTlsStream Connect();
	void AssertOnWorkQueue();
	void ExceptionHandler(boost::exception_ptr exp);
	void Flush();
	void SendRequest(const String& body);
};

}

#endif /* ELASTICSEARCHWRITER_H */
