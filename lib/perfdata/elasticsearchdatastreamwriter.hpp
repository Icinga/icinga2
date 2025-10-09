/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ELASTICSEARCHDATASTREAMWRITER_H
#define ELASTICSEARCHDATASTREAMWRITER_H

#include <boost/beast/http/verb.hpp>

#include "base/configobject.hpp"
#include "base/dictionary.hpp"
#include "base/workqueue.hpp"
#include "base/timer.hpp"
#include "base/tlsstream.hpp"
#include "config/expression.hpp"
#include "icinga/checkable.hpp"
#include "icinga/checkresult.hpp"
#include "icinga/macroprocessor.hpp"
#include "remote/url.hpp"

#include "perfdata/elasticsearchdatastreamwriter-ti.hpp"

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

	void ValidateHostTagsTemplate(const Lazy<Array::Ptr> &lvalue, const ValidationUtils &utils) override;
	void ValidateServiceTagsTemplate(const Lazy<Array::Ptr> &lvalue, const ValidationUtils &utils) override;
	void ValidateHostLabelsTemplate(const Lazy<Dictionary::Ptr> &lvalue, const ValidationUtils &utils) override;
	void ValidateServiceLabelsTemplate(const Lazy<Dictionary::Ptr> &lvalue, const ValidationUtils &utils) override;
	void ValidateFilter(const Lazy<Value> &lvalue, const ValidationUtils &utils) override;

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;
	Value TrySend(Url::Ptr url, String body);

private:
	WorkQueue m_WorkQueue{10000000, 1};
	boost::signals2::connection m_HandleCheckResults;
	Timer::Ptr m_FlushTimer;
	bool m_Paused = false;

	// This buffer should only be accessed from the worker thread.
	// Every other access will lead to a race-condition.
	std::vector<EcsDocument::Ptr> m_DataBuffer;

	std::uint64_t m_DocumentsSent = 0;
	std::uint64_t m_DocumentsFailed = 0;
	std::atomic_uint64_t m_ItemsFilteredOut = 0;

	Expression::Ptr m_CompiledFilter;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void ManageIndexTemplate();

	Dictionary::Ptr ExtractPerfData(const Checkable::Ptr checkable, const Array::Ptr& perfdata);
	Array::Ptr ExtractTemplateTags(const MacroProcessor::ResolverList resolvers, const Array::Ptr tags_tmpl,
		const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	Dictionary::Ptr ExtractTemplateLabels(const MacroProcessor::ResolverList resolvers, const Dictionary::Ptr labels_tmpl,
		const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	String ExtractDatastreamNamespace(const MacroProcessor::ResolverList resolvers, const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr);
	bool Filter(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);

	OptionalTlsStream Connect();
	void AssertOnWorkQueue();
	void ExceptionHandler(boost::exception_ptr exp);
	void Flush();
	void SendRequest(const String& body);

	void ValidateTagsTemplate(Array::Ptr tags);
	void ValidateLabelsTemplate(Dictionary::Ptr tags);
};

class StatusCodeException : public std::runtime_error
{
public:
	StatusCodeException(int status_code, String message, String body)
		: std::runtime_error(std::move(message)), m_StatusCode(status_code), m_Body(std::move(body)) {}

	int GetStatusCode() const { return m_StatusCode; }
	const String& GetBody() const { return m_Body; }

private:
	int m_StatusCode;
	String m_Body;
};

}

#endif /* ELASTICSEARCHWRITER_H */
