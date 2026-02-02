/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ELASTICSEARCHDATASTREAMWRITER_H
#define ELASTICSEARCHDATASTREAMWRITER_H

#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/status.hpp>

#include "base/configobject.hpp"
#include "base/dictionary.hpp"
#include "base/shared-object.hpp"
#include "base/workqueue.hpp"
#include "base/timer.hpp"
#include "config/expression.hpp"
#include "icinga/checkable.hpp"
#include "icinga/checkresult.hpp"
#include "icinga/macroprocessor.hpp"
#include "remote/url.hpp"
#include "perfdata/perfdatawriterconnection.hpp"

#include "perfdata/elasticsearchdatastreamwriter-ti.hpp"

namespace icinga
{

class EcsDocument final : public SharedObject {
	public:
		DECLARE_PTR_TYPEDEFS(EcsDocument);

		EcsDocument() = default;
		EcsDocument(String index, Dictionary::Ptr document);

		String GetIndex() const { return m_Index; }
		void SetIndex(const String& index) { m_Index = index; }
		Dictionary::Ptr GetDocument() const { return m_Document; }
		void SetDocument(Dictionary::Ptr document) { m_Document = document; }

	private:
		String m_Index;
		Dictionary::Ptr m_Document;
};

class ElasticsearchDatastreamWriter final : public ObjectImpl<ElasticsearchDatastreamWriter>
{
public:
	DECLARE_OBJECT(ElasticsearchDatastreamWriter);
	DECLARE_OBJECTNAME(ElasticsearchDatastreamWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static String FormatTimestamp(double ts);

	void ValidateHostTagsTemplate(const Lazy<Array::Ptr> &lvalue, const ValidationUtils &utils) override;
	void ValidateServiceTagsTemplate(const Lazy<Array::Ptr> &lvalue, const ValidationUtils &utils) override;
	void ValidateHostLabelsTemplate(const Lazy<Dictionary::Ptr> &lvalue, const ValidationUtils &utils) override;
	void ValidateServiceLabelsTemplate(const Lazy<Dictionary::Ptr> &lvalue, const ValidationUtils &utils) override;
	void ValidateFilter(const Lazy<Value> &lvalue, const ValidationUtils &utils) override;

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;
	Value TrySend(const Url::Ptr& url, String body);

private:
	WorkQueue m_WorkQueue{10000000, 1};
	boost::signals2::connection m_HandleCheckResults;
	Timer::Ptr m_FlushTimer;
	bool m_Paused = false;

	PerfdataWriterConnection::Ptr m_Connection;

	// This buffer should only be accessed from the worker thread.
	// Every other access will lead to a race-condition.
	std::vector<EcsDocument::Ptr> m_DataBuffer;

	std::uint64_t m_DocumentsSent = 0;
	std::uint64_t m_DocumentsFailed = 0;
	std::atomic_uint64_t m_ItemsFilteredOut = 0;

	Expression::Ptr m_CompiledFilter;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void ManageIndexTemplate();

	Dictionary::Ptr ExtractPerfData(const Checkable::Ptr& checkable, const Array::Ptr& perfdata);
	String ExtractDatastreamNamespace(const MacroProcessor::ResolverList& resolvers, const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr);
	bool Filter(const Checkable::Ptr& checkable);

	void AssertOnWorkQueue();
	void Flush();
	void SendRequest(const String& body);

	void ValidateTagsTemplate(const Array::Ptr& tags, const String& attrName);
	void ValidateLabelsTemplate(const Dictionary::Ptr& labels, const String& attrName);
};

class StatusCodeException : public std::runtime_error
{
public:
	StatusCodeException(boost::beast::http::status statusCode, String message, String body)
		: std::runtime_error((message + " (HTTP " + std::to_string(static_cast<unsigned>(statusCode)) + ")").CStr()),
		  m_StatusCode(statusCode), m_Body(std::move(body)) {}

	boost::beast::http::status GetStatusCode() const { return m_StatusCode; }
	const String& GetBody() const { return m_Body; }

private:
	boost::beast::http::status m_StatusCode;
	String m_Body;
};

}

#endif /* ELASTICSEARCHDATASTREAM_H */
